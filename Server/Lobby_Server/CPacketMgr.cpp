#include "pch.h"
#include "CPacketMgr.h"
#include "CMatchMgr.h"
#include "CNetworkMgr.h"

namespace wod_server {
	std::unique_ptr<CPacketMgr> CPacketMgr::m_instance;

	bool CPacketMgr::Initialize()
	{
		try {
			m_packetfunc.insert({ CS_LOGIN, [this](BASE_PACKET* p, CClient* c) {LoginPacket(p, c); } });
			m_packetfunc.insert({ CS_MOVE ,[this](BASE_PACKET* p, CClient* c) {MovePacket(p, c); } });
			m_packetfunc.insert({ CS_MATCH ,[this](BASE_PACKET* p, CClient* c) {MatchPacket(p, c); } });
			m_packetfunc.insert({ CS_CHAT, [this](BASE_PACKET* p, CClient* c) {ChatPacket(p, c); } });
			m_packetfunc.insert({ CS_ROTATE, [this](BASE_PACKET* p, CClient* c) {RotatePacket(p, c); } });
			m_packetfunc.insert({ CS_CUSTOMIZE, [this](BASE_PACKET* p, CClient* c) {CustomizePacket(p, c); } });
			m_packetfunc.insert({ CS_SHOP, [this](BASE_PACKET* p, CClient* c) {ShopPacket(p, c); } });
			m_packetfunc.insert({ CS_SIGN_UP, [this](BASE_PACKET* p, CClient* c) {SignUpPacket(p, c); } });
			m_packetfunc.insert({ CS_CHANGE_CHANNEL, [this](BASE_PACKET* p, CClient* c) {ChangeChannelPacket(p, c); } });
			m_packetfunc.insert({ CS_PORT_NUM, [this](BASE_PACKET* p, CClient* c) {PortNumPacket(p, c); } });
			m_packetfunc.insert({ CS_CHANGE_NODE, [this](BASE_PACKET* p, CClient* c) {ChangeNodePacket(p, c); } });
			m_packetfunc.insert({ CS_STAKE_TOKEN, [this](BASE_PACKET* p, CClient* c) {StakeTokenPacket(p, c); } });
			m_packetfunc.insert({ CS_DUMMY_CLIENT, [this](BASE_PACKET* p, CClient* c) {DummyClientPacket(p,c); } });
			m_packetfunc.insert({ CS_CREATE_TRANSACTION, [this](BASE_PACKET* p, CClient* c) {CreateTransactionPacket(p,c); } });
			m_packetfunc.insert({ CS_OPEN_AUCTION, [this](BASE_PACKET* p, CClient* c) {OpenAuctionPacket(p,c); } });
			m_packetfunc.insert({ CS_OPEN_CUSTOMIZE, [this](BASE_PACKET* p, CClient* c) {OpenCustomizePacket(p,c); } });
			m_packetfunc.insert({ CS_OPEN_BLOCKCHAIN, [this](BASE_PACKET* p, CClient* c) {OpenBlockChainPacket(p,c); } });
			m_packetfunc.insert({ CS_GET_AUCTION_INFO, [this](BASE_PACKET* p, CClient* c) {GetAuctionInfoPacket(p,c); } });
			m_packetfunc.insert({ CS_REGISTER_AUCTION, [this](BASE_PACKET* p, CClient* c) {RegisterAuctionPacket(p,c); } });
			m_packetfunc.insert({ CS_BUY_AUCTION, [this](BASE_PACKET* p, CClient* c) {BuyAuctionPacket(p,c); } });
			m_packetfunc.insert({ CS_RTT, [this](BASE_PACKET* p, CClient* c) {RTTPacket(p,c); } });

			//For Test
			m_packetfunc.insert({ CS_TEST_CHANGE_SERVER, [this](BASE_PACKET* p, CClient* c) {ChangeServerPacket(p, c); } });

			m_randomEngine = std::mt19937(std::random_device{}());

			std::fstream in("Resource/CustomizeNumber.txt");
			std::string temp;
			for (int i = 0; i < m_shopMaxNums.size(); ++i) {
				in >> temp;
				if (temp == "#" || temp == "") {
					in >> temp;
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

	void CPacketMgr::Packet_Exec(BASE_PACKET* packet, CClient* client)
	{
		if (m_packetfunc.contains(packet->type)) {
			m_packetfunc[packet->type](packet, client);
		}
		else {
			LogPrinter::PrintMsg(static_cast<int>(packet->type) + ": Undefined Packet Type");
		}
	}

	void CPacketMgr::LoginPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_LOGIN_PACKET* p = reinterpret_cast<CS_LOGIN_PACKET*>(packet);

		if (!GetPlayerInfo(p->name, p->password, client))
			return;

		client->stateLock.lock();
		client->SetState(CL_STATE::ST_LOBBY);
		client->stateLock.unlock();

		client->SetName(p->name);
		client->GetPacketSender()->SendLoginInfoPacket(client->GetID(), client->GetTransform()->GetPos(), client->GetModelCustomize());

		//Initialize client position
		client->ProcessUpdate();
		client->GetPacketSender()->SendMovePacket(client->GetID(), client->GetTransform()->GetPos(), client->GetTransform()->GetDir());

		for (CClient* otherClient : CUserMgr::GetInstance()->GetChannelClients(client->GetChannel())) {
			if (otherClient == nullptr)
				break;
			if (otherClient->GetID() == client->GetID())
				continue;
			client->GetPacketSender()->SendModelCustomizePacket(otherClient->GetID(), otherClient->GetModelCustomize());
		}
	}

	void CPacketMgr::MovePacket(BASE_PACKET* packet, CClient* client)
	{
		CS_MOVE_PACKET* p = reinterpret_cast<CS_MOVE_PACKET*>(packet);

		char befDir = client->GetTransform()->GetDir();
		client->GetTransform()->SetDir(p->direction);

		if (p->direction == 0) {
			client->GetPhysics()->SetVelocity(vec3(0, 0, 0));
		}

		// dir이 0라면 Update수행 안하는 것으로 간주
		if (befDir == 0)
		{
			client->SetUpdateTime();
			client->GetJobQueue()->PushJob([client]() {	client->ProcessUpdate(); });
			GPacketJobQueue->AddSessionQueue(client);
		}
	}

	void CPacketMgr::MatchPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_MATCH_PACKET* p = reinterpret_cast<CS_MATCH_PACKET*>(packet);

		if (p->match) {
			LogPrinter::PrintMsg("MatchPacket");
			{
				std::lock_guard ll{ client->stateLock };
				client->SetState(CL_STATE::ST_MATCHING);
			}
			match::GetInstance()->RegisterToQue(static_cast<int>(client->GetPacketSender()->GetSession()->GetSocket()), p->character, p->match_time);

			if (match::GetInstance()->GetMatch()) {
				LG_MATCH_START_PACKET matchStartPkt = { sizeof(LG_MATCH_START_PACKET), LG_MATCH_START, static_cast<short>(m_matchNum) };
				network::GetInstance()->GetGameServer()->Send(&matchStartPkt);

				for (int i = 0; i < MATCH_PLAYER - 1; ++i) {
					int clientID = match::GetInstance()->GetMatchPlayer();
					CClient* matchClient = CUserMgr::GetInstance()->GetClient(clientID);
					matchClient->stateLock.lock_shared();
					if (CL_STATE::ST_MATCHING != matchClient->GetState()) {
						CUserMgr::GetInstance()->GetClient(clientID)->stateLock.unlock_shared();
						--i;
						continue;
					}
					else {
						CUserMgr::GetInstance()->GetClient(clientID)->stateLock.unlock_shared();
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
					CUserMgr::GetInstance()->GetClient(clientID)->stateLock.lock_shared();
					if (CL_STATE::ST_MATCHING != matchClient->GetState()) {
						matchClient->stateLock.unlock_shared();
						continue;
					}
					else {
						matchClient->stateLock.unlock_shared();
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
				std::lock_guard ll{ client->stateLock };
				client->SetState(CL_STATE::ST_LOBBY);
			}
			match::GetInstance()->DecreaseMatchPlayers(p->character);
		}
	}

	void CPacketMgr::ChatPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_CHAT_PACKET* p = reinterpret_cast<CS_CHAT_PACKET*>(packet);

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
			int channel = client->GetChannel();
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

	void CPacketMgr::RotatePacket(BASE_PACKET* packet, CClient* client)
	{
		CS_ROTATE_PACKET* p = reinterpret_cast<CS_ROTATE_PACKET*>(packet);
		client->GetTransform()->SetLook(vec3(p->lookX, p->lookY, p->lookZ));
		client->GetTransform()->SetRight(vec3(p->rightX, p->rightY, p->rightZ));

		for (int nearClientID : client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendRotatePacket(client->GetID(), client->GetTransform()->GetLook(), client->GetTransform()->GetRight());
		}
	}

	void CPacketMgr::CustomizePacket(BASE_PACKET* packet, CClient* client)
	{
		CS_CUSTOMIZE_PACKET* p = reinterpret_cast<CS_CUSTOMIZE_PACKET*>(packet);

		client->SetModelCustomize(p->model);
		client->GetPacketSender()->SendModelCustomizePacket(client->GetID(), client->GetModelCustomize());

		for (int nearClientID : client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(nearClientID)->GetPacketSender()->SendModelCustomizePacket(client->GetID(), client->GetModelCustomize());
		}
	}

	void CPacketMgr::ShopPacket(BASE_PACKET* packet, CClient* client)
	{
		SC_SHOP_PACKET* p = reinterpret_cast<SC_SHOP_PACKET*>(packet);
		SHOP_TYPE type = static_cast<SHOP_TYPE>(p->customizePart);

		PlayerInfo playerInfo = client->GetPlayerInfo();

		if (playerInfo.tokenNum < SHOP_BUY_TOKEN_NUM) {
			client->GetPacketSender()->SendShopPacket(-1, -1);
		}
		else {
			std::uniform_int_distribution<> uid(0, m_shopMaxNums[p->customizePart]);

			int num = uid(m_randomEngine);
			client->GetPacketSender()->SendShopPacket(p->customizePart, num);
			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(client->GetName(), 
				std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SHOP_BUY, p->customizePart, num));

			playerInfo.tokenNum -= SHOP_BUY_TOKEN_NUM;
			client->SetPlayerInfo(playerInfo);
			client->GetPacketSender()->SendTokenNumPacket(playerInfo.tokenNum);

			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(client->GetName()), -SHOP_BUY_TOKEN_NUM, 
				std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

			//Add BlockChain Code
			std::time_t t = std::time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char buffer[TIME_SIZE + 5];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);

			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, client->GetName(), NAME_SIZE);
			transaction.token = -SHOP_BUY_TOKEN_NUM;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
		}
	}

	void CPacketMgr::SignUpPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_SIGN_UP_PACKET* p = reinterpret_cast<CS_SIGN_UP_PACKET*>(packet);
		if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->CheckIdExists(p->name)) {
			if (network::GetInstance()->GetDataBaseThread()->GetDataBase()->SignUp(p->name, p->password)) {
				PlayerInfo info = client->GetPlayerInfo();
				if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetPlayerInfo(p->name, p->password, info)) {
					//Server Error
					client->GetPacketSender()->SendLoginFailPacket(3);
					return;
				}
				else {
					//Sign Up Complete
					client->SetName(p->name);
					client->SetPlayerInfo(info);
					client->GetPacketSender()->SendTokenNumPacket(info.tokenNum);
				}

			}
			else {
				//Server Err
				client->GetPacketSender()->SendLoginFailPacket(3);
				return;
			}
		}
		else {
			//ID Exists
			client->GetPacketSender()->SendLoginFailPacket(2);
			return;
		}

		client->stateLock.lock();
		client->SetState(CL_STATE::ST_LOBBY);
		client->stateLock.unlock();

		//Initialize client position
		client->Move();
		client->GetPacketSender()->SendMovePacket(client->GetID(), client->GetTransform()->GetPos(), client->GetTransform()->GetDir());

		CUserMgr::GetInstance()->RegisterClientToChannel(client);
		client->GetPacketSender()->SendLoginInfoPacket(client->GetID(), client->GetTransform()->GetPos(), client->GetModelCustomize());

		for (CClient* otherClient : CUserMgr::GetInstance()->GetChannelClients(client->GetChannel())) {
			if (otherClient == nullptr)
				break;
			if (otherClient->GetID() == client->GetID())
				continue;
			client->GetPacketSender()->SendModelCustomizePacket(otherClient->GetID(), otherClient->GetModelCustomize());
		}

		client->ProcessUpdate();
	}

	void CPacketMgr::RegisterAuctionPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_REGISTER_AUCTION_PACKET* p = reinterpret_cast<CS_REGISTER_AUCTION_PACKET*>(packet);
		network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(p->name), std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_REGISTER_AUCTION, p->customizeType, p->customizeNum, p->buyPrice));
	}

	void CPacketMgr::GetAuctionInfoPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_GET_AUCTION_INFO_PACKET* p = reinterpret_cast<CS_GET_AUCTION_INFO_PACKET*>(packet);
		std::vector<AuctionInfo> auctionInfos = network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetAuctionInfo(p->pageNum);
		client->GetPacketSender()->SendAuctionInfoPacket(auctionInfos);
	}

	void CPacketMgr::ChangeChannelPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_CHANGE_CHANNEL_PACKET* p = reinterpret_cast<CS_CHANGE_CHANNEL_PACKET*>(packet);

		if (CUserMgr::GetInstance()->ChangeChannel(client->GetChannel(), p->channel, client)) {
			client->GetPacketSender()->SendChangeChannelPacket(false, p->channel);
			client->GetPacketSender()->SendLoginInfoPacket(client->GetID(), client->GetTransform()->GetPos(), client->GetModelCustomize());
		}
		else {
			client->GetPacketSender()->SendChangeChannelPacket(true, client->GetChannel());
		}
	}

	void CPacketMgr::PortNumPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_PORT_NUM_PACKET* p = reinterpret_cast<CS_PORT_NUM_PACKET*>(packet);

		network::GetInstance()->GetP2PNetwork()->RegisterPortNum(client->GetIP(), p->portNum);

		std::unordered_map<std::string, int> peerInfos = network::GetInstance()->GetP2PNetwork()->GetPeerInfos();

		if (client->IsFullNode()) {
			for (const auto& info : peerInfos) {
				if (info.first != client->GetIP())
					client->GetPacketSender()->SendPeerInfoPacket(info.first, info.second);
			}
		}
		else {
			if (peerInfos.size() > 1) {
				for (auto iter = peerInfos.end(); iter != peerInfos.begin();) {
					--iter;
					if (iter->first != client->GetIP()) {
						client->GetPacketSender()->SendPeerInfoPacket(iter->first, iter->second);
						return;
					}
				}
			}
		}
	}

	void CPacketMgr::StakeTokenPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_STAKE_TOKEN_PACKET* p = reinterpret_cast<CS_STAKE_TOKEN_PACKET*>(packet);

		if (p->stakeDays < MIN_STAKE_DAYS) {
			client->GetPacketSender()->SendStakeTokenPacket(true, 0);
			return;
		}
		else if (p->stakeNum < MIN_STAKE_TOKEN) {
			client->GetPacketSender()->SendStakeTokenPacket(true, 1);
			return;
		}

		if (network::GetInstance()->GetDataBaseThread()->GetDataBase()->StakeTokens(client->GetName(), p->stakeNum, p->stakeDays)) {
			client->GetPacketSender()->SendStakeTokenPacket(false, 0);	//Success
			PlayerInfo playerInfo = client->GetPlayerInfo();
			playerInfo.tokenNum -= p->stakeNum;
			client->SetPlayerInfo(playerInfo);
			client->GetPacketSender()->SendTokenNumPacket(playerInfo.tokenNum);

			//Add BlockChain Code
			std::time_t t = std::time(nullptr);
			std::tm time;
			gmtime_s(&time, &t);
			char buffer[TIME_SIZE + 5];
			strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);

			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, client->GetName(), NAME_SIZE);
			transaction.token = -p->stakeNum;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);

			//Send StakeTokenInfo
			int stakedToken = 0;
			int stakedDays = 0;
			int dayCount = 0;
			network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetStakedTokenData(std::string(client->GetName()), stakedToken, stakedDays, dayCount);

			if(stakedToken != 0)
				client->GetPacketSender()->SendStakedTokenInfoPacket(stakedToken, stakedDays - dayCount);
		}
		else {
			client->GetPacketSender()->SendStakeTokenPacket(true, 2);	//Server Error
		}
	}

	void CPacketMgr::ChangeNodePacket(BASE_PACKET* packet, CClient* client)
	{
		CS_CHANGE_NODE_PACKET* p = reinterpret_cast<CS_CHANGE_NODE_PACKET*>(packet);

		if (client->GetPlayerInfo().tokenNum < MIN_STAKE_TOKEN * 100) {
			client->GetPacketSender()->SendChangeNodePacket(true, 0);	//Not Enough Token
		}

		if (!network::GetInstance()->GetDataBaseThread()->GetDataBase()->ChangeNodeType(client->GetName(), p->fullNode)) {
			client->GetPacketSender()->SendChangeNodePacket(true, 1);	//Server Error
		}
		else {
			client->GetPacketSender()->SendChangeNodePacket(false, 0);
		}
	}

	void CPacketMgr::DummyClientPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_DUMMY_CLIENT_PACKET* p = reinterpret_cast<CS_DUMMY_CLIENT_PACKET*>(packet);

		std::string dummyName = "Dummy" + std::to_string(p->id);

		client->stateLock.lock();
		client->SetState(CL_STATE::ST_LOBBY);
		client->stateLock.unlock();

		client->SetName(dummyName.c_str());
		client->GetTransform()->SetPos({ static_cast<float>(rand() % 300), 5.f,static_cast<float>(rand() % 300) });

		client->GetPacketSender()->SendDummyLoginInfo(client->GetID(), client->GetTransform()->GetPos());

		//Initialize client position
		client->ProcessUpdate(true);
	}

	void CPacketMgr::CreateTransactionPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_CREATE_TRANSACTION_PACKET* p = reinterpret_cast<CS_CREATE_TRANSACTION_PACKET*>(packet);

		std::time_t t = std::time(nullptr);
		std::tm time;
		gmtime_s(&time, &t);
		char buffer[TIME_SIZE + 5];
		strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &time);
		int tokenNum = GenerateRandomNumber(100, 1000);

		TransactionData transaction;
		memcpy_s(transaction.name, NAME_SIZE, client->GetName(), NAME_SIZE);
		transaction.token = tokenNum;
		memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

		int playerToken = client->GetPlayerInfo().tokenNum + tokenNum;
		client->SetTokenNum(playerToken);
		client->GetPacketSender()->SendTokenNumPacket(playerToken);
		network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(client->GetName()), tokenNum, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

		network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
	}

	void CPacketMgr::OpenCustomizePacket(BASE_PACKET* packet, CClient* client)
	{
		std::vector<std::tuple<short, short, short>> customizeParts;
		network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetPlayerCustomizeParts(std::string(client->GetName()), customizeParts);

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
		client->GetPacketSender()->SendCustomizePartsPacket(partTypes, customizeNum, counts);
	}

	void CPacketMgr::OpenAuctionPacket(BASE_PACKET* packet, CClient* client)
	{
		int num = network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetAuctionPartsNum();
		client->GetPacketSender()->SendAuctionPartsNumPacket(num);
	}

	void CPacketMgr::OpenBlockChainPacket(BASE_PACKET* packet, CClient* client)
	{
		int stakedToken = 0;
		int stakedDays = 0;
		int dayCount = 0;
		network::GetInstance()->GetDataBaseThread()->GetDataBase()->GetStakedTokenData(std::string(client->GetName()), stakedToken, stakedDays, dayCount);

		client->GetPacketSender()->SendStakedTokenInfoPacket(stakedToken, stakedDays - dayCount);
	}

	void CPacketMgr::BuyAuctionPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_BUY_AUCTION_PACKET* p = reinterpret_cast<CS_BUY_AUCTION_PACKET*>(packet);

		int tokenNum = client->GetPlayerInfo().tokenNum;
		if (tokenNum >= p->buyPrice) {
			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_BUY_AUCTION, std::string(p->sellerName), std::string(client->GetName()), p->customizeType, p->customizeNum, p->buyPrice));
			client->SetTokenNum(tokenNum - p->buyPrice);
			client->GetPacketSender()->SendTokenNumPacket(client->GetPlayerInfo().tokenNum);

			network::GetInstance()->GetDataBaseThread()->RegisterDBEvent(DB_EVENT(std::string(client->GetName()), -p->buyPrice, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

			//If seller is online, send token change
			for (int i = 0; i < MAX_CLIENT / LOBBY_MAX_CLIENT; ++i) {
				for (CClient* cl : CUserMgr::GetInstance()->GetChannelClients(i)) {
					if (cl == nullptr)
						continue;
					cl->stateLock.lock_shared();
					if (cl->GetState() != CL_STATE::ST_LOBBY) {
						cl->stateLock.unlock_shared();
						continue;
					}
					cl->stateLock.unlock_shared();
					if (!strcmp(cl->GetName(), p->sellerName)) {
						cl->SetTokenNum(cl->GetPlayerInfo().tokenNum + p->buyPrice);
						cl->GetPacketSender()->SendTokenNumPacket(cl->GetPlayerInfo().tokenNum);
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
			memcpy_s(transaction.name, NAME_SIZE, client->GetName(), NAME_SIZE);
			transaction.token = -p->buyPrice;
			memcpy_s(transaction.time, NAME_SIZE, buffer, NAME_SIZE);

			network::GetInstance()->GetP2PNetwork()->InsertTransaction(transaction);
		}
	}

	void CPacketMgr::LoginCompletePacket(BASE_PACKET* packet, CClient* client)
	{
		for (int id : client->GetViewList()->GetView()) {
			CUserMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendModelCustomizePacket(client->GetID(), client->GetModelCustomize());
		}
	}

	void CPacketMgr::RTTPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_RTT_PACKET* p = reinterpret_cast<CS_RTT_PACKET*>(packet);

		long long currentTime = std::chrono::high_resolution_clock::now().time_since_epoch().count();
		long long packetDelayTime = (currentTime - p->serverTime) / 2;
		long long timeDifference = p->time - (p->serverTime + packetDelayTime);
		client->GetPacketSender()->SetTimeDifference(timeDifference);
	}

	void CPacketMgr::ChangeServerPacket(BASE_PACKET* packet, CClient* client)
	{
		CS_TEST_CHANGE_SERVER_PACKET* p = reinterpret_cast<CS_TEST_CHANGE_SERVER_PACKET*>(packet);
		if (p->boss) {
			m_matchId = 3;
		}
		LG_MATCH_PACKET lgp = { sizeof(LG_MATCH_PACKET), LG_MATCH_PLAYER, "", static_cast<short>(m_matchNum), m_matchId };
		memcpy_s(lgp.name, NAME_SIZE, client->GetName(), NAME_SIZE); 
		network::GetInstance()->GetGameServer()->Send(&lgp);
		client->GetPacketSender()->SendServerChangePacket(m_matchId);
		client->Disconnect();
	}

	bool CPacketMgr::GetPlayerInfo(char name[NAME_SIZE], char password[NAME_SIZE], CClient* client)
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
		client->GetPacketSender()->SendTokenNumPacket(2000);

#endif

		return true;
	}
	
}