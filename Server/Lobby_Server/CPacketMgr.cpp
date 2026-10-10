#include "pch.h"
#include "CPacketMgr.h"
#include "CMatchMgr.h"
#include "CNetworkMgr.h"

namespace wod_server {
	std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

	bool CPacketMgr::Initialize()
	{
		try {
			m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* _p, CClient* _c) {LoginPacket(_p, _c); } });
			m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* _p, CClient* _c) {MovePacket(_p, _c); } });
			m_packetfunc.insert({ CS_MATCH ,[this](BASE_PACKET* _p, CClient* _c) {MatchPacket(_p, _c); } });
			m_packetfunc.insert({ CS_CHAT, [this](BASE_PACKET* _p, CClient* _c) {ChatPacket(_p, _c); } });
			m_packetfunc.insert({ CS_ROTATE, [this](BASE_PACKET* _p, CClient* _c) {RotatePacket(_p, _c); } });
			m_packetfunc.insert({ CS_CUSTOMIZE, [this](BASE_PACKET* _p, CClient* _c) {CustomizePacket(_p, _c); } });
			m_packetfunc.insert({ CS_SHOP, [this](BASE_PACKET* _p, CClient* _c) {ShopPacket(_p, _c); } });
			m_packetfunc.insert({ CS_SIGN_UP, [this](BASE_PACKET* _p, CClient* _c) {SignUpPacket(_p, _c); } });
			m_packetfunc.insert({ CS_CHANGE_CHANNEL, [this](BASE_PACKET* _p, CClient* _c) {ChangeChannelPacket(_p, _c); } });
			m_packetfunc.insert({ CS_PORT_NUM, [this](BASE_PACKET* _p, CClient* _c) {PortNumPacket(_p, _c); } });
			m_packetfunc.insert({ CS_CHANGE_NODE, [this](BASE_PACKET* _p, CClient* _c) {ChangeNodePacket(_p, _c); } });
			m_packetfunc.insert({ CS_STAKE_TOKEN, [this](BASE_PACKET* _p, CClient* _c) {StakeTokenPacket(_p, _c); } });
			m_packetfunc.insert({ CS_DUMMY_CLIENT, [this](BASE_PACKET* _p, CClient* _c) {DummyClientPacket(_p,_c); } });
			m_packetfunc.insert({ CS_CREATE_TRANSACTION, [this](BASE_PACKET* _p, CClient* _c) {CreateTransactionPacket(_p,_c); } });
			m_packetfunc.insert({ CS_OPEN_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {OpenAuctionPacket(_p,_c); } });
			m_packetfunc.insert({ CS_OPEN_CUSTOMIZE, [this](BASE_PACKET* _p, CClient* _c) {OpenCustomizePacket(_p,_c); } });
			m_packetfunc.insert({ CS_OPEN_BLOCKCHAIN, [this](BASE_PACKET* _p, CClient* _c) {OpenBlockChainPacket(_p,_c); } });
			m_packetfunc.insert({ CS_GET_AUCTION_INFO, [this](BASE_PACKET* _p, CClient* _c) {GetAuctionInfoPacket(_p,_c); } });
			m_packetfunc.insert({ CS_REGISTER_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {RegisterAuctionPacket(_p,_c); } });
			m_packetfunc.insert({ CS_BUY_AUCTION, [this](BASE_PACKET* _p, CClient* _c) {BuyAuctionPacket(_p,_c); } });
			m_packetfunc.insert({ CS_RTT, [this](BASE_PACKET* _p, CClient* _c) {RTTPacket(_p,_c); } });

			//For Test
			m_packetfunc.insert({ CS_TEST_CHANGE_SERVER, [this](BASE_PACKET* _p, CClient* _c) {ChangeServerPacket(_p, _c); } });

			m_randomEngine = std::mt19937(std::random_device{}());

			std::fstream in("Resource/CustomizeNumber.txt");
            if (!in) throw std::runtime_error("missing Resource/CustomizeNumber.txt");
			std::string temp;
			for (int i = 0; i < m_shopMaxNums.size(); ++i) {
                if (!(in >> temp)) throw std::runtime_error("incomplete CustomizeNumber.txt");
				if (temp == "#" || temp == "") {
	                if (!(in >> temp)) throw std::runtime_error("incomplete CustomizeNumber.txt");
					--i;
					continue;
				}
				m_shopMaxNums[i] = std::stoi(temp) - 1;
			}

			return true;
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg(ex.what());
			return false;
		}

	}

	bool CPacketMgr::Release()
	{
		return true;
	}

	void CPacketMgr::Packet_Exec(BASE_PACKET* _packet, CClient* _client)
	{
		if (m_packetfunc.contains(_packet->type)) {
			m_packetfunc[_packet->type](_packet, _client);
		}
		else {
			LogPrinter::PrintMsg(static_cast<int>(_packet->type) + ": Undefined Packet Type");
		}
	}

	void CPacketMgr::LoginPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(_packet);

		if (!GetPlayerInfo(p->name, p->password, _client))
			return;

		_client->m_stateLock.lock();
		_client->SetState(CL_STATE::ST_LOBBY);
		_client->m_stateLock.unlock();

		_client->SetName(p->name);
		_client->GetPacketSender()->SendLoginInfoPacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetModelCustomize());

		//Initialize client position
		_client->ProcessUpdate();
		_client->GetPacketSender()->SendMovePacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetTransform()->GetDir());

		for (CClient* otherClient : CUserMgr::GetInstance()->GetChannelClients(_client->GetChannel())) {
			if (otherClient == nullptr)
				break;
			if (otherClient->GetID() == _client->GetID())
				continue;
			_client->GetPacketSender()->SendModelCustomizePacket(otherClient->GetID(), otherClient->GetModelCustomize());
		}
	}

	void CPacketMgr::MovePacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(_packet);

		char befDir = _client->GetTransform()->GetDir();
		_client->GetTransform()->SetDir(p->direction);

		if (p->direction == 0) {
			_client->GetPhysics()->SetVelocity(vec3(0, 0, 0));
		}

		// dir이 0라면 Update수행 안하는 것으로 간주
		if (befDir == 0)
		{
			_client->SetUpdateTime();
			_client->GetJobQueue()->PushJob([_client]() {	_client->ProcessUpdate(); });
			GPacketJobQueue->AddSessionQueue(_client);
		}
	}

	void CPacketMgr::MatchPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_MATCH_PACKET* p = reinterpret_cast<CS_MATCH_PACKET*>(_packet);

		if (p->match) {
			LogPrinter::PrintMsg("MatchPacket");
			{
				std::lock_guard ll{ _client->m_stateLock };
				_client->SetState(CL_STATE::ST_MATCHING);
			}
			match::GetInstance()->RegisterToQue(static_cast<int>(_client->GetPacketSender()->GetSession()->GetSocket()), p->character, p->match_time);

			if (match::GetInstance()->GetMatch()) {
				LG_MATCH_START_PACKET matchStartPkt = { sizeof(LG_MATCH_START_PACKET), LG_MATCH_START, static_cast<short>(m_matchNum) };
				network::GetInstance()->GetGameServer()->Send(&matchStartPkt);

				for (int i = 0; i < MATCH_PLAYER - 1; ++i) {
					int clientID = match::GetInstance()->GetMatchPlayer();
					CClient* matchClient = CUserMgr::GetInstance()->GetClient(clientID);
					matchClient->m_stateLock.lock_shared();
					if (CL_STATE::ST_MATCHING != matchClient->GetState()) {
						CUserMgr::GetInstance()->GetClient(clientID)->m_stateLock.unlock_shared();
						--i;
						continue;
					}
					else {
						CUserMgr::GetInstance()->GetClient(clientID)->m_stateLock.unlock_shared();
						LG_MATCH_PACKET lgp = { sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), i, matchClient->GetModelCustomize() };
						memcpy_s(lgp.name, NAME_SIZE, matchClient->GetName(), NAME_SIZE);
						network::GetInstance()->GetGameServer()->Send(&lgp);
						matchClient->GetPacketSender()->SendMatchPacket(i);
						matchClient->Disconnect();
					}
				}
				while (true) {
					int clientID = match::GetInstance()->GetMatchBoss();
					CClient* matchClient = CUserMgr::GetInstance()->GetClient(clientID);
					CUserMgr::GetInstance()->GetClient(clientID)->m_stateLock.lock_shared();
					if (CL_STATE::ST_MATCHING != matchClient->GetState()) {
						matchClient->m_stateLock.unlock_shared();
						continue;
					}
					else {
						matchClient->m_stateLock.unlock_shared();
						LG_MATCH_PACKET lgp = { sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), 3, matchClient->GetModelCustomize() };
						memcpy_s(lgp.name, NAME_SIZE, matchClient->GetName(), NAME_SIZE);
						network::GetInstance()->GetGameServer()->Send(&lgp);
						matchClient->GetPacketSender()->SendMatchPacket(3);
						matchClient->Disconnect();
						break;
					}
				}
				LogPrinter::PrintMsg("Matching");
				m_matchNum++;
			}
		}
		else {
			LogPrinter::PrintMsg("MATCH Cancel");
			{
				std::lock_guard ll{ _client->m_stateLock };
				_client->SetState(CL_STATE::ST_LOBBY);
			}
			match::GetInstance()->DecreaseMatchPlayers(p->character);
		}
	}

	void CPacketMgr::ChatPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(_packet);

		SC_CHAT_PACKET scp;
		scp.size = sizeof(SC_CHAT_PACKET);
		scp.type = SC_CHAT;
		scp.chatType = p->chatType;
		wcscpy_s(scp.chat, p->chat);
		memcpy_s(scp.name, NAME_SIZE, p->name, NAME_SIZE);

		switch (p->chatType) {
		case 0: {
			//broadcast to all clients
			for (const auto& [socketNum, channelClient] : CUserMgr::GetInstance()->GetAllClient()) {
				if (nullptr == channelClient) {
					break;
				}
				channelClient->GetPacketSender()->GetSession()->Send(&scp);
			}
			break;
		}
		case 1: {
			// channel
			int channel = _client->GetChannel();
			for (const auto& channelClient : CUserMgr::GetInstance()->GetChannelClients(channel)) {
				if (nullptr == channelClient) {
					break;
				}
				channelClient->GetPacketSender()->GetSession()->Send(&scp);
			}
		}
			  break;
		case 2:
			// party
			break;
		default:
			LogPrinter::PrintMsg("Wrong chat type");
			break;
		}

#ifdef TEST
		std::cout << "Client " << p->id << " Sended : ";
		std::wcout << p->chat << std::endl;
#endif // TEST
	}

	void CPacketMgr::RotatePacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_ROTATE_PACKET* p = reinterpret_cast<CS_ROTATE_PACKET*>(_packet);
		_client->GetTransform()->SetLook(vec3(p->lookX, p->lookY, p->lookZ));
		_client->GetTransform()->SetRight(vec3(p->rightX, p->rightY, p->rightZ));

		for (int nearClientID : _client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendRotatePacket(_client->GetID(), _client->GetTransform()->GetLook(), _client->GetTransform()->GetRight());
		}
	}

	void CPacketMgr::CustomizePacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_CUSTOMIZE_PACKET* p = reinterpret_cast<CS_CUSTOMIZE_PACKET*>(_packet);

		_client->SetModelCustomize(p->model);
		_client->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());

		for (int nearClientID : _client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());
		}
	}

	void CPacketMgr::ShopPacket(BASE_PACKET* _packet, CClient* _client)
	{
		SC_SHOP_PACKET* p = reinterpret_cast<SC_SHOP_PACKET*>(_packet);
		SHOP_TYPE type = static_cast<SHOP_TYPE>(p->customizePart);

		PlayerInfo playerInfo = _client->GetPlayerInfo();

		if (playerInfo.m_tokenNum < SHOP_BUY_TOKEN_NUM) {
			_client->GetPacketSender()->SendShopPacket(-1, -1);
		}
		else {
			std::uniform_int_distribution<> uid(0, m_shopMaxNums[p->customizePart]);

			int num = uid(m_randomEngine);
			_client->GetPacketSender()->SendShopPacket(p->customizePart, num);
			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(_client->GetName(),
				std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SHOP_BUY, p->customizePart, num));

			playerInfo.m_tokenNum -= SHOP_BUY_TOKEN_NUM;
			_client->SetPlayerInfo(playerInfo);
			_client->GetPacketSender()->SendTokenNumPacket(playerInfo.m_tokenNum);

			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(_client->GetName()), -SHOP_BUY_TOKEN_NUM,
				std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

			//Add BlockChain Code
			std::time_t t = std::time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char buffer[TIME_SIZE + 5];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);

			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
			transaction.token = -SHOP_BUY_TOKEN_NUM;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
		}
	}

	void CPacketMgr::SignUpPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_SIGN_UP_PACKET* p = reinterpret_cast<CS_SIGN_UP_PACKET*>(_packet);
		if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->CheckIdExists(p->name)) {
			if (network::GetInstance()->GetDataBaseThread()->GetDataBase()->SignUp(p->name, p->password)) {
				PlayerInfo info = _client->GetPlayerInfo();
				if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetPlayerInfo(p->name, p->password, info)) {
					//Server Error
					_client->GetPacketSender()->SendLoginFailPacket(3);
					return;
				}
				else {
					//Sign Up Complete
					_client->SetName(p->name);
					_client->SetPlayerInfo(info);
					_client->GetPacketSender()->SendTokenNumPacket(info.m_tokenNum);
				}

			}
			else {
				//Server Err
				_client->GetPacketSender()->SendLoginFailPacket(3);
				return;
			}
		}
		else {
			//ID Exists
			_client->GetPacketSender()->SendLoginFailPacket(2);
			return;
		}

		_client->m_stateLock.lock();
		_client->SetState(CL_STATE::ST_LOBBY);
		_client->m_stateLock.unlock();

		//Initialize client position
		_client->Move();
		_client->GetPacketSender()->SendMovePacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetTransform()->GetDir());

		CUserMgr::GetInstance()->RegisterClientToChannel(_client);
		_client->GetPacketSender()->SendLoginInfoPacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetModelCustomize());

		for (CClient* otherClient : CUserMgr::GetInstance()->GetChannelClients(_client->GetChannel())) {
			if (otherClient == nullptr)
				break;
			if (otherClient->GetID() == _client->GetID())
				continue;
			_client->GetPacketSender()->SendModelCustomizePacket(otherClient->GetID(), otherClient->GetModelCustomize());
		}

		_client->ProcessUpdate();
	}

	void CPacketMgr::RegisterAuctionPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_REGISTER_AUCTION_PACKET* p = reinterpret_cast<CS_REGISTER_AUCTION_PACKET*>(_packet);
		network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(p->name), std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_REGISTER_AUCTION, p->customizeType, p->customizeNum, p->buyPrice));
	}

	void CPacketMgr::GetAuctionInfoPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_GET_AUCTION_INFO_PACKET* p = reinterpret_cast<CS_GET_AUCTION_INFO_PACKET*>(_packet);
		std::vector<AuctionInfo> auctionInfos = network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetAuctionInfo(p->pageNum);
		_client->GetPacketSender()->SendAuctionInfoPacket(auctionInfos);
	}

	void CPacketMgr::ChangeChannelPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_CHANGE_CHANNEL_PACKET* p = reinterpret_cast<CS_CHANGE_CHANNEL_PACKET*>(_packet);

		if (CUserMgr::GetInstance()->ChangeChannel(_client->GetChannel(), p->channel, _client)) {
			_client->GetPacketSender()->SendChangeChannelPacket(false, p->channel);
			_client->GetPacketSender()->SendLoginInfoPacket(_client->GetID(), _client->GetTransform()->GetPos(), _client->GetModelCustomize());
		}
		else {
			_client->GetPacketSender()->SendChangeChannelPacket(true, _client->GetChannel());
		}
	}

	void CPacketMgr::PortNumPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_PORT_NUM_PACKET* p = reinterpret_cast<CS_PORT_NUM_PACKET*>(_packet);

		network::GetInstance()->GetP2PNetwork()->RegisterPortNum(_client->GetIP(), p->portNum);

		std::unordered_map<std::string, int> peerInfos = network::GetInstance()->GetP2PNetwork()->GetPeerInfos();

		if (_client->IsFullNode()) {
			for (const auto& info : peerInfos) {
				if (info.first != _client->GetIP())
					_client->GetPacketSender()->SendPeerInfoPacket(info.first, info.second);
			}
		}
		else {
			if (peerInfos.size() > 1) {
				for (auto iter = peerInfos.end(); iter != peerInfos.begin();) {
					--iter;
					if (iter->first != _client->GetIP()) {
						_client->GetPacketSender()->SendPeerInfoPacket(iter->first, iter->second);
						return;
					}
				}
			}
		}
	}

	void CPacketMgr::StakeTokenPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_STAKE_TOKEN_PACKET* p = reinterpret_cast<CS_STAKE_TOKEN_PACKET*>(_packet);

		if (p->stakeDays < MIN_STAKE_DAYS) {
			_client->GetPacketSender()->SendStakeTokenPacket(true, 0);
			return;
		}
		else if (p->stakeNum < MIN_STAKE_TOKEN) {
			_client->GetPacketSender()->SendStakeTokenPacket(true, 1);
			return;
		}

		if (network::GetInstance()->GetDataBaseThread()->GetDataBase()->StakeTokens(_client->GetName(), p->stakeNum, p->stakeDays)) {
			_client->GetPacketSender()->SendStakeTokenPacket(false, 0);	//Success
			PlayerInfo playerInfo = _client->GetPlayerInfo();
			playerInfo.m_tokenNum -= p->stakeNum;
			_client->SetPlayerInfo(playerInfo);
			_client->GetPacketSender()->SendTokenNumPacket(playerInfo.m_tokenNum);

			//Add BlockChain Code
			std::time_t t = std::time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char buffer[TIME_SIZE + 5];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);

			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
			transaction.token = -p->stakeNum;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);

			//Send StakeTokenInfo
			int stakedToken = 0;
			int stakedDays = 0;
			int dayCount = 0;
			network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetStakedTokenData(std::string(_client->GetName()), stakedToken, stakedDays, dayCount);

			if(stakedToken != 0)
				_client->GetPacketSender()->SendStakedTokenInfoPacket(stakedToken, stakedDays - dayCount);
		}
		else {
			_client->GetPacketSender()->SendStakeTokenPacket(true, 2);	//Server Error
		}
	}

	void CPacketMgr::ChangeNodePacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_CHANGE_NODE_PACKET* p = reinterpret_cast<CS_CHANGE_NODE_PACKET*>(_packet);

		if (_client->GetPlayerInfo().m_tokenNum < MIN_STAKE_TOKEN * 100) {
			_client->GetPacketSender()->SendChangeNodePacket(true, 0);	//Not Enough Token
		}

		if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->ChangeNodeType(_client->GetName(), p->fullNode)) {
			_client->GetPacketSender()->SendChangeNodePacket(true, 1);	//Server Error
		}
		else {
			_client->GetPacketSender()->SendChangeNodePacket(false, 0);
		}
	}

	void CPacketMgr::DummyClientPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_DUMMY_CLIENT_PACKET* p = reinterpret_cast<CS_DUMMY_CLIENT_PACKET*>(_packet);

		std::string dummyName = "Dummy" + std::to_string(p->id);

		_client->m_stateLock.lock();
		_client->SetState(CL_STATE::ST_LOBBY);
		_client->m_stateLock.unlock();

		_client->SetName(dummyName.c_str());
		_client->GetTransform()->SetPos({ static_cast<float>(rand() % 300), 5.f,static_cast<float>(rand() % 300) });

		_client->GetPacketSender()->SendDummyLoginInfo(_client->GetID(), _client->GetTransform()->GetPos());

		//Initialize client position
		_client->ProcessUpdate(true);
	}

	void CPacketMgr::CreateTransactionPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_CREATE_TRANSACTION_PACKET* p = reinterpret_cast<CS_CREATE_TRANSACTION_PACKET*>(_packet);

		std::time_t t = std::time(nullptr);
		std::tm time;
		gmtime_s(&time, &t);
		char buffer[TIME_SIZE + 5];
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
		int tokenNum = GenerateRandomNumber(100, 1000);

		TransactionData transaction;
		memcpy_s(transaction.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
		transaction.token = tokenNum;
		memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

		int playerToken = _client->GetPlayerInfo().m_tokenNum + tokenNum;
		_client->SetTokenNum(playerToken);
		_client->GetPacketSender()->SendTokenNumPacket(playerToken);
		network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(_client->GetName()), tokenNum, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

		network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
	}

	void CPacketMgr::OpenCustomizePacket(BASE_PACKET* _packet, CClient* _client)
	{
		std::vector<std::tuple<short, short, short>> customizeParts;
		network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetPlayerCustomizeParts(std::string(_client->GetName()), customizeParts);

		short partTypes[CUSTOMIZE_PART_NUM_FROM_SERVER];
		short customizeNum[CUSTOMIZE_PART_NUM_FROM_SERVER];
		short counts[CUSTOMIZE_PART_NUM_FROM_SERVER];

		for (int i = 0; i < CUSTOMIZE_PART_NUM_FROM_SERVER; ++i) {
			if (i < customizeParts.size()) {
				partTypes[i] = std::get<0>(customizeParts[i]);
				customizeNum[i] = std::get<1>(customizeParts[i]);
				counts[i] = std::get<2>(customizeParts[i]);
			}
			else {
				partTypes[i] = 200;
				customizeNum[i] = 200;
				counts[i] = 200;
			}
		}
		_client->GetPacketSender()->SendCustomizePartsPacket(partTypes, customizeNum, counts);
	}

	void CPacketMgr::OpenAuctionPacket(BASE_PACKET* _packet, CClient* _client)
	{
		int num = network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetAuctionPartsNum();
		_client->GetPacketSender()->SendAuctionPartsNumPacket(num);
	}

	void CPacketMgr::OpenBlockChainPacket(BASE_PACKET* _packet, CClient* _client)
	{
		int stakedToken = 0;
		int stakedDays = 0;
		int dayCount = 0;
		network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetStakedTokenData(std::string(_client->GetName()), stakedToken, stakedDays, dayCount);

		_client->GetPacketSender()->SendStakedTokenInfoPacket(stakedToken, stakedDays - dayCount);
	}

	void CPacketMgr::BuyAuctionPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_BUY_AUCTION_PACKET* p = reinterpret_cast<CS_BUY_AUCTION_PACKET*>(_packet);

		int tokenNum = _client->GetPlayerInfo().m_tokenNum;
		if (tokenNum >= p->buyPrice) {
			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_BUY_AUCTION, std::string(p->sellerName), std::string(_client->GetName()), p->customizeType, p->customizeNum, p->buyPrice));
			_client->SetTokenNum(tokenNum - p->buyPrice);
			_client->GetPacketSender()->SendTokenNumPacket(_client->GetPlayerInfo().m_tokenNum);

			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(_client->GetName()), -p->buyPrice, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

			//If seller is online, send token change
			for (int i = 0; i < MAX_CLIENT / LOBBY_MAX_CLIENT; ++i) {
				for (CClient* cl : CUserMgr::GetInstance()->GetChannelClients(i)) {
					if (cl == nullptr)
						continue;
					cl->m_stateLock.lock_shared();
					if (cl->GetState() != CL_STATE::ST_LOBBY) {
						cl->m_stateLock.unlock_shared();
						continue;
					}
					cl->m_stateLock.unlock_shared();
					if (!strcmp(cl->GetName(), p->sellerName)) {
						cl->SetTokenNum(cl->GetPlayerInfo().m_tokenNum + p->buyPrice);
						cl->GetPacketSender()->SendTokenNumPacket(cl->GetPlayerInfo().m_tokenNum);
						break;
					}
				}
			}
			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(p->sellerName), p->buyPrice, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

			//Add BlockChain Code
			std::time_t t = std::time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char buffer[TIME_SIZE + 5];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);

			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
			transaction.token = -p->buyPrice;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
		}
	}

	void CPacketMgr::LoginCompletePacket(BASE_PACKET* _packet, CClient* _client)
	{
		for (int id : _client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendModelCustomizePacket(_client->GetID(), _client->GetModelCustomize());
		}
	}

	void CPacketMgr::RTTPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_RTT_PACKET* p = reinterpret_cast<CS_RTT_PACKET*>(_packet);

		long long currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		long long packetDelayTime = (currentTime - p->serverTime) / 2;
		long long timeDifference = p->time - (p->serverTime + packetDelayTime);
		_client->GetPacketSender()->SetTimeDifference(timeDifference);
	}

	void CPacketMgr::ChangeServerPacket(BASE_PACKET* _packet, CClient* _client)
	{
		CS_TEST_CHANGE_SERVER_PACKET* p = reinterpret_cast<CS_TEST_CHANGE_SERVER_PACKET*>(_packet);
		if (p->boss) {
			m_matchId = 3;
		}
		LG_MATCH_PACKET lgp = { sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), m_matchId };
		memcpy_s(lgp.name, NAME_SIZE, _client->GetName(), NAME_SIZE);
		network::GetInstance()->GetGameServer()->Send(&lgp);
		_client->GetPacketSender()->SendServerChangePacket(m_matchId);
		_client->Disconnect();
	}

	bool CPacketMgr::GetPlayerInfo(char _name[NAME_SIZE], char _password[NAME_SIZE], CClient* _client)
	{
#ifdef WITH_DATABASE
		CDataBase* database = network::GetInstance()->GetDataBaseThread()->GetDataBase();
		if (!database->CheckIdExists(name)) {
			client->GetPacketSender()->SendLoginFailPacket(0);
			return false;
		}
		else if (!database->CheckPassword(name, password)) {
			client->GetPacketSender()->SendLoginFailPacket(1);
			return false;
		}
		else {
			PlayerInfo info = client->GetPlayerInfo();
			if (!database->GetPlayerInfo(name, password, info)) {
				LogPrinter::PrintMsg("Failed To Get Info From Database");
				return false;
			}
			client->SetPlayerInfo(info);
			client->GetPacketSender()->SendTokenNumPacket(info.tokenNum);
		}
		bool isFullNode = false;
		if (!database->GetFullNodeInfo(name, isFullNode)) {
			LogPrinter::PrintMsg("Failed To Get Info From Database");
			return false;
		}
		client->SetFullNode(isFullNode);
		client->GetPacketSender()->SendFullNodePacket(isFullNode);
#else
		_client->GetPacketSender()->SendTokenNumPacket(2000);

#endif

		return true;
	}

}