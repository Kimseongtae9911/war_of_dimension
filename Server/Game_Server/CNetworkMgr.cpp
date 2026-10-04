#include "pch.h"
#include "SockAddr.h"
#include "SocketUtil.h"
#include "Resource.h"
#include "ClientInfos.h"
#include "NpcInfos.h"
#include "CMatch.h"

namespace wod_server {
	constexpr float MONSTER_HP_HEAL_PERCENTAGE = 0.05f;

	std::unique_ptr<CNetworkMgr> CNetworkMgr::m_instance;

	bool CNetworkMgr::Initialize()
	{
		try {
			m_handle = std::make_shared<TCPSocket>();
			m_LobbyServer = std::make_shared<Session>(true);
			m_skillTimer = new CSkillTimer;

			m_iocpfunc.insert({ OP_TYPE::OP_ACCEPT, [this](int id, int bytes, OverlapEx* over_ex) {Accept(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, OverlapEx* over_ex) {Recv(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, OverlapEx* over_ex) {Send(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_DISCONNECT, [this](int id, int bytes, OverlapEx* over_ex) {Disconnect(id, bytes, over_ex); } });

			m_iocpfunc.insert({ OP_TYPE::OP_CONNECT_UPDATE, [this](int id, int bytes, OverlapEx* over_ex) {ConnectUpdate(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_READY_UPDATE, [this](int id, int bytes, OverlapEx* over_ex) {ReadyUpdate(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_LOADING_UPDATE,[this](int id, int bytes, OverlapEx* over_ex) {LoadingUpdate(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_MATCH_UPDATE,[this](int id, int bytes, OverlapEx* over_ex) {MatchUpdate(id, bytes, over_ex); } });			

			m_iocpfunc.insert({ OP_TYPE::OP_NPC_ACTIVE,[this](int id, int bytes, OverlapEx* over_ex) {NpcActive(id, bytes, over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_MATCH_FINISH, [this](int id, int bytes, OverlapEx* over_ex) {MatchFinish(id, bytes, over_ex); } });			

			//Make SocketPool and Clients
			for (int i = 0; i < MAX_SOCKET; ++i) {
				std::shared_ptr<Session> s = std::make_shared<Session>(true);
				s->SetSocketID(i);
				SocketUtil::socketpool.push(s);
				
				CreateIoCompletionPort(reinterpret_cast<HANDLE>(s->GetSocket()), m_handle->GetHandle(), i, 0);

				CObjectMgr::GetInstance()->MakeClientObject(i);
			}

			for (int i = 0; i < MAX_OVEREX_OBJECT; ++i) {
				OverlapEx* overEx = new OverlapEx;
				Resource::overExPool.push(overEx);
			}

#ifndef LOCAL_TEST
			std::cout << "Input Lobby Server IP: " << std::endl;
			std::cin >> lobbyIP;
#else
			lobbyIP = "127.0.0.1";
#endif

			m_LobbyServer->Connect(lobbyIP);	// Connect To Lobby Server
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_LobbyServer->GetSocket()), m_handle->GetHandle(), LOBBY_SERVER_ID, 0);
			m_LobbyServer->Recv();

			m_handle->Bind(SockAddr(GAME_PORT));
			m_handle->Listen();

			std::shared_ptr<Session> session;
			SocketUtil::socketpool.try_pop(session);
			m_handle->Accept(session);

			GUID op = WSAID_DISCONNECTEX;
			DWORD bytes = 0;
			WSAIoctl(m_handle->GetSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER, &op, sizeof(op), &SocketUtil::DisconnectEx, sizeof(SocketUtil::DisconnectEx), &bytes, NULL, NULL);
			
			m_clientnum = 0;

			return true;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg(ex.what());
			return false;
		}
	}

	bool CNetworkMgr::Release()
	{
		delete m_skillTimer;
		return true;
	}

	void CNetworkMgr::IOCPFunc()
	{
		// Key Value 0~1999 client, 9999 lobby server, 2000~ npcs
		while (true) {
			DWORD bytes;
			ULONG_PTR key;
			WSAOVERLAPPED* over = nullptr;
			int err = GetQueuedCompletionStatus(m_handle->GetHandle(), &bytes, &key, &over, INFINITE);
			OverlapEx* overEx = reinterpret_cast<OverlapEx*>(over);

			if (0 == err) {
				if (OP_TYPE::OP_ACCEPT == overEx->GetOP())
					LogPrinter::PrintMsg("Accept Error");
				else if (OP_TYPE::OP_SEND == overEx->GetOP()) {
					overEx->Reset();
					Resource::overExPool.push(overEx);
				}
				else {
					if (key == LOBBY_SERVER_ID)
						continue;	//Disconnect Lobby
					CObjectMgr::GetInstance()->DisconnectClient(static_cast<int>(key));
				}
				continue;
			}

			// Process GQCS
			auto iter = m_iocpfunc.find(overEx->GetOP());
			if (iter != m_iocpfunc.end()) {
				iter->second(static_cast<int>(key), bytes, overEx);
			}
			else
				LogPrinter::PrintMsg("Wrong Key Value For IOCP Function");
		}
	}

	void CNetworkMgr::TimerFunc()
	{
		while (true) {
			TIMER_EVENT ev;
			auto current_time = TimeUtil::CurTime();
			if (m_timerQueue.try_pop(ev)) {
				if (ev.wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
				switch (ev.eventID) {
				case EVENT_TYPE::EV_CONNECT_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_CONNECT_UPDATE);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_READY_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_READY_UPDATE);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_LOADING_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_LOADING_UPDATE);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_MATCH_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_MATCH_UPDATE);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_STAT_CHANGE:
				{
					if (ev.objID >= NPC_ID) {
						std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(ev.targetID, ev.objID - NPC_ID);
						npc->SetSpeed(npc->GetSpeed() - ev.changeStat.speed);
					}
					else {
						std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
						client->GetStatus()->SetStat(CObjectMgr::GetInstance()->GetClient(ev.objID)->GetStatus()->GetStat() - ev.changeStat);
					}
					break;
				}
				case EVENT_TYPE::EV_SKILL_END:
				{
					CObjectMgr::GetInstance()->GetClient(ev.objID)->SetUsingSkill(false);
					break;
				}
				case EVENT_TYPE::EV_MATCH_FINISH:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_MATCH_FINISH);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_HEALTHMANA_CHANGE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_HEALTHMANA_CHANGE);
					ov->SetSocketID(ev.changeMaxHp);
					ov->SetInfo(ev.changeMaxMp);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				case EVENT_TYPE::EV_NPC_ACTIVE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_NPC_ACTIVE);
					PostQueuedCompletionStatus(network::GetInstance()->GetHandle(), 1, ev.objID, &ov->GetOver());
					break;
				}
				}
				continue;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}


	void CNetworkMgr::Accept(int id, int bytes, OverlapEx* overEx)
	{
		if (m_clientnum >= MAX_CLIENT) {
			LogPrinter::PrintMsg("Max user exceeded");
		}
		else {
			LogPrinter::PrintMsg("Accept");
			CObjectMgr::GetInstance()->InitializeClient(m_handle->GetClientSocket(), overEx->GetSocketID());

			m_clientnum++;
		}
		m_handle->GetOverEx().ResetOver();
		std::shared_ptr<Session> session;
		if (SocketUtil::socketpool.try_pop(session))
			m_handle->Accept(session);
		else {
			while (false == SocketUtil::socketpool.try_pop(session)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			m_handle->Accept(session);
		}
	}

	void CNetworkMgr::Recv(int id, int bytes, OverlapEx* overEx)
	{
		//Recv from Lobby Server
		if (LOBBY_SERVER_ID == id) {
			int remaindata = bytes + m_LobbyServer->GetRemainData();
			char* packet = overEx->GetSendBuf();
			while (remaindata > 0) {
				BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);
				if (p->size <= remaindata) {
					Packet_Exec(p);
					packet += p->size;
					remaindata -= p->size;
				}
				else 
					break;
			}
			m_LobbyServer->SetRemainData(remaindata);
			if (remaindata < 0)
				memmove(overEx->GetSendBuf(), packet, remaindata);
			m_LobbyServer->Recv();
		}
		else {
			//Recv from Clients
			int clientIndex = CObjectMgr::GetInstance()->GetUserIDFromSocket(id);
			CObjectMgr::GetInstance()->GetClient(clientIndex)->RecvProcess(bytes, overEx);
		}
	}

	void CNetworkMgr::Send(int id, int bytes, OverlapEx* overEx)
	{
		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::Disconnect(int id, int bytes, OverlapEx* overEx)
	{
		LogPrinter::PrintMsg("Disconnect");
		CObjectMgr::GetInstance()->RemoveClientFromServer(id);
		m_clientnum -= 1;
		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::ConnectUpdate(int id, int bytes, OverlapEx* overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(id);
		match.ConnectUpdate(id);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::ReadyUpdate(int id, int bytes, OverlapEx* overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(id);
		match.ReadyUpdate(id);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::LoadingUpdate(int id, int bytes, OverlapEx* overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(id);
		match.LoadingUpdate(id);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::MatchUpdate(int id, int bytes, OverlapEx* overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(id);
		match.InGameUpdate(id);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::MonsterHeal(int id, int bytes, OverlapEx* overEx)
	{
		int matchNum = overEx->GetSocketID();
		int npcID = id - NPC_ID;

		CMonster* monster = reinterpret_cast<CMonster*>(CObjectMgr::GetInstance()->GetNpc(matchNum, npcID).get());

		monster->Heal();

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::NpcActive(int id, int bytes, OverlapEx* overEx)
	{
		int match = overEx->GetSocketID(); //NPC matchNum
		auto npc = CObjectMgr::GetInstance()->GetNpc(match, id - NPC_ID);
		int time = static_cast<int>(CGameMgr::GetInstance()->GetGameTime(match) / 60);

		npc->Respawn(time);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::MatchFinish(int id, int bytes, OverlapEx* overEx)
	{
		const auto& playerIDs = CMatchMgr::GetInstance()->GetMatchPlayers(id);
		//Reset Objects, Send MatchEnd Packet
		bool heroWin = static_cast<bool>(CGameMgr::GetInstance()->IsGameOver(id));
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == playerIDs[i])
				continue;
			CObjectMgr::GetInstance()->GetClient(playerIDs[i])->GetPacketSender()->SendMatchEndPacket(heroWin);
		}
		if (-1 != playerIDs[3]) {
			CObjectMgr::GetInstance()->GetClient(playerIDs[3])->GetPacketSender()->SendMatchEndPacket(!heroWin);
		}

		for (int clientID : playerIDs) {
			if (-1 == clientID)
				continue;
			CObjectMgr::GetInstance()->GetClient(clientID)->Disconnect();
		}

		CGameMgr::GetInstance()->Reset(id);

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::HealthManaChange(int id, int bytes, OverlapEx* overEx)
	{
		const auto& changedClient = CObjectMgr::GetInstance()->GetClient(id);
		//socketId == changeMaxHp, info == changeMaxMp
		changedClient->GetStatus()->healthMana.SetMaxHp(changedClient->GetStatus()->healthMana.GetMaxHp() - overEx->GetSocketID());
		changedClient->GetStatus()->healthMana.HealHp(0);
		changedClient->GetStatus()->healthMana.SetMaxMp(changedClient->GetStatus()->healthMana.GetMaxMp() - overEx->GetInfo());
		changedClient->GetStatus()->healthMana.HealMp(0);

		for (int playerID : CMatchMgr::GetInstance()->GetMatchPlayers(changedClient->GetMatchNum())) {
			if (-1 == playerID)
				continue;
			CObjectMgr::GetInstance()->GetClient(playerID)->GetPacketSender()->SendPlayerHealthManaPacket(changedClient->GetMatchId(), changedClient->GetStatus()->healthMana);
		}

		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::Packet_Exec(BASE_PACKET* packet)
	{
		switch (packet->type) {
		case LG_MATCH_START: {
			LG_MATCH_START_PACKET* p = reinterpret_cast<LG_MATCH_START_PACKET*>(packet);
			m_timerQueue.push({ p->match_num, TimeUtil::PassedTimeMSec(1000), EVENT_TYPE::EV_CONNECT_UPDATE, -1 });
		}
		case LG_MATCH_PLAYER: {
			LG_MATCH_PACKET* p = reinterpret_cast<LG_MATCH_PACKET*>(packet);
			CObjectMgr::GetInstance()->RegisterClientToServer(p->name, p->id, p->match_num, p->model);
			break;
		}
		default:
			LogPrinter::PrintMsg(static_cast<int>(packet->type) + ": Undefined Packet From Lobby Server");
			break;
		}
	}

	void CNetworkMgr::InitializeMonster(int matchNum)
	{
		constexpr uint8_t RARE_MONSTER_NUM = 2;

		auto uniquePosIndexs = std::move(RandomUtil::GenerateUniqueRandomNumbers(0, 2, 1));
		auto rarePosIndexs = std::move(RandomUtil::GenerateUniqueRandomNumbers(0, 3, RARE_MONSTER_NUM));
		auto normalPosIndexs = std::move(RandomUtil::GenerateUniqueRandomNumbers(0, 11, 6));
		auto posIter = uniquePosIndexs.begin();
		for (int8_t i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i)
		{
			auto monster = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
			monster->Initialize(*posIter);
			if (i == MAX_MINION)
				posIter = rarePosIndexs.begin();
			else if (i == MAX_MINION + RARE_MONSTER_NUM)
				posIter = normalPosIndexs.begin();
			else
				posIter++;
		}
	}

	void CNetworkMgr::RegisterSkillEvent(const SKILL_EVENT& ev)
	{
		m_skillTimer->PushEvent(ev);
	}
}
