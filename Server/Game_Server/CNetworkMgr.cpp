#include "pch.h"
#include <Protocol/Validation.h>
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
				
				SocketUtil::Runtime().Attach(s->GetSocket(), i);

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
			SocketUtil::Runtime().Attach(m_LobbyServer->GetSocket(), LOBBY_SERVER_ID);
			m_LobbyServer->Recv();

			m_handle->Bind(SockAddr(GAME_PORT));
			m_handle->Listen();

			std::shared_ptr<Session> session;
			SocketUtil::socketpool.try_pop(session);
			m_handle->Accept(session);
			
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
        m_stopping.store(true);
        SocketUtil::Runtime().RequestStop(1);
        wod::core::Completion completion;
        while (SocketUtil::Runtime().Stats().pending && SocketUtil::Runtime().Poll(completion)) {
            auto op = completion.context->operation;
            if (op == wod::core::IoOperation::Send || op == wod::core::IoOperation::Disconnect || op == wod::core::IoOperation::AppEvent)
                Resource::overExPool.push(static_cast<OverlapEx*>(completion.context));
        }
        SocketUtil::Runtime().Finish();
        LogPrinter::PrintMsg("ServerCore stop pending=0 sockets=0 leased=" + std::to_string(Resource::overExPool.Leased()));
        Resource::overExPool.Clear();
        delete m_skillTimer; m_skillTimer = nullptr;
        std::shared_ptr<Session> session;
        while(SocketUtil::socketpool.try_pop(session)) {}
        while(Resource::sessionPool.try_pop(session)) {}
        m_LobbyServer.reset();
        m_handle.reset(); return true;
	}

	void CNetworkMgr::IOCPFunc()
	{

        try {
        wod::core::Completion completion;
        while (SocketUtil::Runtime().Poll(completion)) {
            auto* over = static_cast<OverlapEx*>(completion.context);
            const auto operation = over->operation;
            if (m_stopping.load() || SocketUtil::Runtime().IsStopping()) {
                if (operation == wod::core::IoOperation::Send || operation == wod::core::IoOperation::Disconnect || operation == wod::core::IoOperation::AppEvent)
                    Resource::overExPool.push(over);
                continue;
            }
            if (completion.error || (operation == wod::core::IoOperation::Receive && completion.bytes == 0)) {
                if (operation == wod::core::IoOperation::Send) { Resource::overExPool.push(over); continue; }
                if (operation == wod::core::IoOperation::Accept) {
                    m_workerFailed.store(true);
                    LogPrinter::PrintMsg("Accept failed: " + std::to_string(completion.error));
                    wod::core::ProcessStopSignal::Request(GetCurrentProcessId()); continue;
                }
                if (operation != wod::core::IoOperation::Disconnect) {
                    if (completion.key == LOBBY_SERVER_ID) { LogPrinter::PrintMsg("Server link closed"); continue; }
                    CObjectMgr::GetInstance()->DisconnectClient(static_cast<int>(completion.key));
                    continue;
                }
            }

            auto found = m_iocpfunc.find(over->GetOP());
            if (found != m_iocpfunc.end()) {
                if (operation==wod::core::IoOperation::AppEvent && over->HasSessionGeneration()) {
                    bool handled=false;
                    auto session=CObjectMgr::GetInstance()->GetClient(static_cast<int>(completion.key))->GetPacketSender()->GetSession();
                    session->WithGeneration(over->GetSessionGeneration(),[&] {
                        handled=true; found->second(static_cast<int>(completion.key),static_cast<int>(completion.bytes),over);
                    });
                    if (!handled) Resource::overExPool.push(over);
                } else found->second(static_cast<int>(completion.key),static_cast<int>(completion.bytes),over);
            }
            else {
                LogPrinter::PrintMsg("Unknown application completion");
                if (operation == wod::core::IoOperation::AppEvent) Resource::overExPool.push(over);
            }
        }

        } catch (const std::exception& error) {
            m_workerFailed.store(true);
            LogPrinter::PrintMsg(std::string("IOCP worker failed: ") + error.what());
            wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
        }
	}

	void CNetworkMgr::TimerFunc()
	{
		while (!m_stopping.load()) {
			TIMER_EVENT ev;
			auto current_time = TimeUtil::CurTime();
			if (m_timerQueue.try_pop(ev)) {
				if (ev.wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
                if (ev.checkSessionGeneration) {
                    auto session=CObjectMgr::GetInstance()->GetClient(ev.objID)->GetPacketSender()->GetSession();
                    session->WithGeneration(ev.sessionGeneration,[&] { ProcessTimerEvent(ev); });
                } else ProcessTimerEvent(ev);
				continue;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}


    void CNetworkMgr::RegisterTimerEvent(const TIMER_EVENT& event) {
        if (m_stopping.load()) return;
        auto captured=event;
        if (event.objID>=0 && event.objID<MAX_CLIENT &&
            (event.eventID==EVENT_TYPE::EV_STAT_CHANGE || event.eventID==EVENT_TYPE::EV_SKILL_END || event.eventID==EVENT_TYPE::EV_HEALTHMANA_CHANGE)) {
            captured.sessionGeneration=CObjectMgr::GetInstance()->GetClient(event.objID)->GetPacketSender()->GetSession()->Generation();
            captured.checkSessionGeneration=true;
        }
        m_timerQueue.push(captured);
    }
    void CNetworkMgr::ProcessTimerEvent(const TIMER_EVENT& ev) {
				switch (ev.eventID) {
				case EVENT_TYPE::EV_CONNECT_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_CONNECT_UPDATE);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_READY_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_READY_UPDATE);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_LOADING_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_LOADING_UPDATE);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_MATCH_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_MATCH_UPDATE);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
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
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_HEALTHMANA_CHANGE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_HEALTHMANA_CHANGE);
                    ov->SetSessionGeneration(ev.sessionGeneration);
					ov->SetSocketID(ev.changeMaxHp);
					ov->SetInfo(ev.changeMaxMp);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_NPC_ACTIVE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_NPC_ACTIVE);
                    ov->SetSocketID(ev.targetID);
					if (!SocketUtil::Runtime().Post(ev.objID, *ov)) Resource::overExPool.push(ov);
					break;
				}
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

        if (id != LOBBY_SERVER_ID) {
            int client = CObjectMgr::GetInstance()->GetUserIDFromSocket(id);
            CObjectMgr::GetInstance()->GetClient(client)->RecvProcess(bytes, overEx); return;
        }
        std::vector<wod::core::FrameDecoder::Frame> frames;
        if (!m_LobbyServer->Decode(bytes, *overEx, frames)) { LogPrinter::PrintMsg("Invalid lobby frame"); SocketUtil::Runtime().Close(m_LobbyServer->GetSocket()); return; }
        for (auto& frame : frames) {
            if (!wod::protocol::Validate(frame, wod::protocol::Endpoint::LobbyToGame)) { LogPrinter::PrintMsg("Invalid lobby packet"); SocketUtil::Runtime().Close(m_LobbyServer->GetSocket()); return; }
            Packet_Exec(reinterpret_cast<BASE_PACKET*>(frame.data()));
        }
        m_LobbyServer->Recv();

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
			if (p->match_num < 0 || p->match_num >= MAX_MATCH) {
				LogPrinter::PrintMsg("Invalid lobby match index"); return;
			}
			m_timerQueue.push({ p->match_num, TimeUtil::PassedTimeMSec(1000), EVENT_TYPE::EV_CONNECT_UPDATE, -1 });
			break;
		}
		case LG_MATCH_PLAYER: {
			LG_MATCH_PACKET* p = reinterpret_cast<LG_MATCH_PACKET*>(packet);
			if (p->match_num < 0 || p->match_num >= MAX_MATCH || p->id < 0 || p->id >= MAX_PLAYER) {
				LogPrinter::PrintMsg("Invalid lobby player index"); return;
			}
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
        // CSV가 제공하는 공통 위치 수 안에서만 그룹별 중복 없는 index를 고른다.
        const auto spawnGroup = [matchNum](int begin, int count) {
            size_t positions = SIZE_MAX;
            for (int i=begin; i<begin+count; ++i) {
                const auto npc = CObjectMgr::GetInstance()->GetNpc(matchNum,i);
                const auto csv = NpcCsvMgr::GetInstance()->GetNpcCsv(npc->GetNpcType());
                if (!csv) throw std::runtime_error("missing NPC CSV for spawn");
                positions = (std::min)(positions,(std::min)(csv->respawnPos.size(),csv->respawnLook.size()));
            }
            if (positions < static_cast<size_t>(count)) throw std::runtime_error("insufficient NPC spawn positions");
            auto indices = RandomUtil::GenerateUniqueRandomNumbers(0,static_cast<int>(positions)-1,count);
            auto position = indices.begin();
            for (int i=begin; i<begin+count; ++i,++position)
                CObjectMgr::GetInstance()->GetNpc(matchNum,i)->Initialize(*position);
        };
        spawnGroup(MAX_MINION,1);
        spawnGroup(MAX_MINION+1,2);
        spawnGroup(MAX_MINION+3,6);
	}

	void CNetworkMgr::RegisterSkillEvent(const SKILL_EVENT& ev)
	{
		m_skillTimer->PushEvent(ev);
	}
}
