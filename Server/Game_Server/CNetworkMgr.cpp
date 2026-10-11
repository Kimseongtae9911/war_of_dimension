#include "pch.h"
#include <Protocol/Validation.h>
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

			m_iocpfunc.insert({ OP_TYPE::OP_ACCEPT, [this](int _id, int _bytes, OverlapEx* _over_ex) {Accept(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int _id, int _bytes, OverlapEx* _over_ex) {Recv(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int _id, int _bytes, OverlapEx* _over_ex) {Send(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_DISCONNECT, [this](int _id, int _bytes, OverlapEx* _over_ex) {Disconnect(_id, _bytes, _over_ex); } });

			m_iocpfunc.insert({ OP_TYPE::OP_CONNECT_UPDATE, [this](int _id, int _bytes, OverlapEx* _over_ex) {ConnectUpdate(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_READY_UPDATE, [this](int _id, int _bytes, OverlapEx* _over_ex) {ReadyUpdate(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_LOADING_UPDATE,[this](int _id, int _bytes, OverlapEx* _over_ex) {LoadingUpdate(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_MATCH_UPDATE,[this](int _id, int _bytes, OverlapEx* _over_ex) {MatchUpdate(_id, _bytes, _over_ex); } });

			m_iocpfunc.insert({ OP_TYPE::OP_NPC_ACTIVE,[this](int _id, int _bytes, OverlapEx* _over_ex) {NpcActive(_id, _bytes, _over_ex); } });
			m_iocpfunc.insert({ OP_TYPE::OP_MATCH_FINISH, [this](int _id, int _bytes, OverlapEx* _over_ex) {MatchFinish(_id, _bytes, _over_ex); } });

			//Make SocketPool and Clients
			for (int i = 0; i < MAX_SOCKET; ++i) {
				std::shared_ptr<Session> s = std::make_shared<Session>(true);
				s->SetSocketID(i);
				Resource::m_acceptSessionPool.push(s);

				NetworkRuntime::Get().Attach(s->GetSocket(), i);

				CObjectMgr::GetInstance()->MakeClientObject(i);
			}

			for (int i = 0; i < MAX_OVEREX_OBJECT; ++i) {
				OverlapEx* overEx = new OverlapEx;
				Resource::m_overExPool.push(overEx);
			}

#ifndef LOCAL_TEST
			std::cout << "Input Lobby Server IP: " << std::endl;
			std::cin >> lobbyIP;
#else
			m_lobbyIP = "127.0.0.1";
#endif

			m_LobbyServer->Connect(m_lobbyIP);	// Connect To Lobby Server
			NetworkRuntime::Get().Attach(m_LobbyServer->GetSocket(), LOBBY_SERVER_ID);
			m_LobbyServer->Recv();

			m_handle->Bind(SockAddr(GAME_PORT));
			m_handle->Listen();

			std::shared_ptr<Session> session;
			Resource::m_acceptSessionPool.try_pop(session);
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
        NetworkRuntime::Get().RequestStop(1);
        wod::core::Completion completion;
        while (NetworkRuntime::Get().Stats().m_pending && NetworkRuntime::Get().Poll(completion)) {
            auto op = completion.m_context->m_operation;
            if (op == wod::core::IoOperation::Send || op == wod::core::IoOperation::Disconnect || op == wod::core::IoOperation::AppEvent)
                Resource::m_overExPool.push(static_cast<OverlapEx*>(completion.m_context));
        }
        NetworkRuntime::Get().Finish();
        LogPrinter::PrintMsg("ServerCore stop pending=0 sockets=0 leased=" + std::to_string(Resource::m_overExPool.Leased()));
        Resource::m_overExPool.Clear();
        delete m_skillTimer; m_skillTimer = nullptr;
        std::shared_ptr<Session> session;
        while(Resource::m_acceptSessionPool.try_pop(session)) {}
        while(Resource::m_sessionPool.try_pop(session)) {}
        m_LobbyServer.reset();
        m_handle.reset(); return true;
	}

	void CNetworkMgr::IOCPFunc()
	{

        try {
        wod::core::Completion completion;
        while (NetworkRuntime::Get().Poll(completion)) {
            auto* over = static_cast<OverlapEx*>(completion.m_context);
            const auto operation = over->m_operation;
            if (m_stopping.load() || NetworkRuntime::Get().IsStopping()) {
                if (operation == wod::core::IoOperation::Send || operation == wod::core::IoOperation::Disconnect || operation == wod::core::IoOperation::AppEvent)
                    Resource::m_overExPool.push(over);
                continue;
            }
            if (completion.m_error || (operation == wod::core::IoOperation::Receive && completion.m_bytes == 0)) {
                if (operation == wod::core::IoOperation::Send) { Resource::m_overExPool.push(over); continue; }
                if (operation == wod::core::IoOperation::Accept) {
                    m_workerFailed.store(true);
                    LogPrinter::PrintMsg("Accept failed: " + std::to_string(completion.m_error));
                    wod::core::ProcessStopSignal::Request(GetCurrentProcessId()); continue;
                }
                if (operation != wod::core::IoOperation::Disconnect) {
                    if (completion.m_key == LOBBY_SERVER_ID) { LogPrinter::PrintMsg("Server link closed"); continue; }
                    CObjectMgr::GetInstance()->DisconnectClient(static_cast<int>(completion.m_key));
                    continue;
                }
            }

            auto found = m_iocpfunc.find(over->GetOP());
            if (found != m_iocpfunc.end()) {
                if (operation==wod::core::IoOperation::AppEvent && over->HasSessionGeneration()) {
                    bool handled=false;
                    auto session=CObjectMgr::GetInstance()->GetClient(static_cast<int>(completion.m_key))->GetPacketSender()->GetSession();
                    session->WithGeneration(over->GetSessionGeneration(),[&] {
                        handled=true; found->second(static_cast<int>(completion.m_key),static_cast<int>(completion.m_bytes),over);
                    });
                    if (!handled) Resource::m_overExPool.push(over);
                } else found->second(static_cast<int>(completion.m_key),static_cast<int>(completion.m_bytes),over);
            }
            else {
                LogPrinter::PrintMsg("Unknown application completion");
                if (operation == wod::core::IoOperation::AppEvent) Resource::m_overExPool.push(over);
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
				if (ev.m_wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::sleep_for(std::chrono::milliseconds(1));
					continue;
				}
                if (ev.m_checkSessionGeneration) {
                    auto session=CObjectMgr::GetInstance()->GetClient(ev.m_objID)->GetPacketSender()->GetSession();
                    session->WithGeneration(ev.m_sessionGeneration,[&] { ProcessTimerEvent(ev); });
                } else ProcessTimerEvent(ev);
				continue;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	}


    void CNetworkMgr::RegisterTimerEvent(const TIMER_EVENT& _event) {
        if (m_stopping.load()) return;
        auto captured=_event;
        if (_event.m_objID>=0 && _event.m_objID<MAX_CLIENT &&
            (_event.m_eventID==EVENT_TYPE::EV_STAT_CHANGE || _event.m_eventID==EVENT_TYPE::EV_SKILL_END || _event.m_eventID==EVENT_TYPE::EV_HEALTHMANA_CHANGE)) {
            captured.m_sessionGeneration=CObjectMgr::GetInstance()->GetClient(_event.m_objID)->GetPacketSender()->GetSession()->Generation();
            captured.m_checkSessionGeneration=true;
        }
        m_timerQueue.push(captured);
    }
    void CNetworkMgr::ProcessTimerEvent(const TIMER_EVENT& _ev) {
				switch (_ev.m_eventID) {
				case EVENT_TYPE::EV_CONNECT_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_CONNECT_UPDATE);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_READY_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_READY_UPDATE);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_LOADING_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_LOADING_UPDATE);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_MATCH_UPDATE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_MATCH_UPDATE);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_STAT_CHANGE:
				{
					if (_ev.m_objID >= NPC_ID) {
						std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_ev.m_targetID, _ev.m_objID - NPC_ID);
						npc->SetSpeed(npc->GetSpeed() - _ev.m_changeStat.m_speed);
					}
					else {
						std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
						client->GetStatus()->SetStat(CObjectMgr::GetInstance()->GetClient(_ev.m_objID)->GetStatus()->GetStat() - _ev.m_changeStat);
					}
					break;
				}
				case EVENT_TYPE::EV_SKILL_END:
				{
					CObjectMgr::GetInstance()->GetClient(_ev.m_objID)->SetUsingSkill(false);
					break;
				}
				case EVENT_TYPE::EV_MATCH_FINISH:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_MATCH_FINISH);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_HEALTHMANA_CHANGE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_HEALTHMANA_CHANGE);
                    ov->SetSessionGeneration(_ev.m_sessionGeneration);
					ov->SetSocketID(_ev.m_changeMaxHp);
					ov->SetInfo(_ev.m_changeMaxMp);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				case EVENT_TYPE::EV_NPC_ACTIVE:
				{
					OverlapEx* ov = Resource::GetOverObjectFromPool();
					ov->SetOP(OP_TYPE::OP_NPC_ACTIVE);
                    ov->SetSocketID(_ev.m_targetID);
					if (!NetworkRuntime::Get().Post(_ev.m_objID, *ov)) Resource::m_overExPool.push(ov);
					break;
				}
				}
    }

	void CNetworkMgr::Accept(int _id, int _bytes, OverlapEx* _overEx)
	{
		if (m_clientnum >= MAX_CLIENT) {
			LogPrinter::PrintMsg("Max user exceeded");
		}
		else {
			LogPrinter::PrintMsg("Accept");
			CObjectMgr::GetInstance()->InitializeClient(m_handle->GetClientSocket(), _overEx->GetSocketID());

			m_clientnum++;
		}
		m_handle->GetOverEx().ResetOver();
		std::shared_ptr<Session> session;
		if (Resource::m_acceptSessionPool.try_pop(session))
			m_handle->Accept(session);
		else {
			while (false == Resource::m_acceptSessionPool.try_pop(session)) {
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
			m_handle->Accept(session);
		}
	}

	void CNetworkMgr::Recv(int _id, int _bytes, OverlapEx* _overEx)
	{

        if (_id != LOBBY_SERVER_ID) {
            int client = CObjectMgr::GetInstance()->GetUserIDFromSocket(_id);
            CObjectMgr::GetInstance()->GetClient(client)->Receive(_bytes, _overEx); return;
        }
        std::vector<wod::core::FrameDecoder::Frame> frames;
        if (!m_LobbyServer->Decode(_bytes, *_overEx, frames)) { LogPrinter::PrintMsg("Invalid lobby frame"); NetworkRuntime::Get().Close(m_LobbyServer->GetSocket()); return; }
        for (auto& frame : frames) {
            if (!wod::protocol::Validate(frame, wod::protocol::Endpoint::LobbyToGame)) { LogPrinter::PrintMsg("Invalid lobby packet"); NetworkRuntime::Get().Close(m_LobbyServer->GetSocket()); return; }
            Packet_Exec(reinterpret_cast<BASE_PACKET*>(frame.data()));
        }
        m_LobbyServer->Recv();

	}

	void CNetworkMgr::Send(int _id, int _bytes, OverlapEx* _overEx)
	{
        Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::Disconnect(int _id, int _bytes, OverlapEx* _overEx)
	{
		LogPrinter::PrintMsg("Disconnect");
		CObjectMgr::GetInstance()->RemoveClientFromServer(_id);
		m_clientnum -= 1;
		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::ConnectUpdate(int _id, int _bytes, OverlapEx* _overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(_id);
		match.ConnectUpdate(_id);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::ReadyUpdate(int _id, int _bytes, OverlapEx* _overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(_id);
		match.ReadyUpdate(_id);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::LoadingUpdate(int _id, int _bytes, OverlapEx* _overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(_id);
		match.LoadingUpdate(_id);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::MatchUpdate(int _id, int _bytes, OverlapEx* _overEx)
	{
		auto& match = CMatchMgr::GetInstance()->GetMatch(_id);
		match.InGameUpdate(_id);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::MonsterHeal(int _id, int _bytes, OverlapEx* _overEx)
	{
		int matchNum = _overEx->GetSocketID();
		int npcID = _id - NPC_ID;

		CMonster* monster = reinterpret_cast<CMonster*>(CObjectMgr::GetInstance()->GetNpc(matchNum, npcID).get());

		monster->Heal();

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::NpcActive(int _id, int _bytes, OverlapEx* _overEx)
	{
		int match = _overEx->GetSocketID(); //NPC matchNum
		auto npc = CObjectMgr::GetInstance()->GetNpc(match, _id - NPC_ID);
		int time = static_cast<int>(CGameMgr::GetInstance()->GetGameTime(match) / 60);

		npc->Respawn(time);
		CMatchMgr::GetInstance()->GetMatch(match).ObserveNpcActivation(_id - NPC_ID);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::MatchFinish(int _id, int _bytes, OverlapEx* _overEx)
	{
		const auto& playerIDs = CMatchMgr::GetInstance()->GetMatchPlayers(_id);
		//Reset Objects, Send MatchEnd Packet
		bool heroWin = static_cast<bool>(CGameMgr::GetInstance()->IsGameOver(_id));
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

		CGameMgr::GetInstance()->Reset(_id);

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::HealthManaChange(int _id, int _bytes, OverlapEx* _overEx)
	{
		const auto& changedClient = CObjectMgr::GetInstance()->GetClient(_id);
		//socketId == changeMaxHp, info == changeMaxMp
		changedClient->GetStatus()->m_healthMana.SetMaxHp(changedClient->GetStatus()->m_healthMana.GetMaxHp() - _overEx->GetSocketID());
		changedClient->GetStatus()->m_healthMana.HealHp(0);
		changedClient->GetStatus()->m_healthMana.SetMaxMp(changedClient->GetStatus()->m_healthMana.GetMaxMp() - _overEx->GetInfo());
		changedClient->GetStatus()->m_healthMana.HealMp(0);

		for (int playerID : CMatchMgr::GetInstance()->GetMatchPlayers(changedClient->GetMatchNum())) {
			if (-1 == playerID)
				continue;
			CObjectMgr::GetInstance()->GetClient(playerID)->GetPacketSender()->SendPlayerHealthManaPacket(changedClient->GetMatchId(), changedClient->GetStatus()->m_healthMana);
		}

		Resource::m_overExPool.push(_overEx);
	}

	void CNetworkMgr::Packet_Exec(BASE_PACKET* _packet)
	{
		switch (_packet->type) {
		case LG_MATCH_START: {
			LG_MATCH_START_PACKET* p = reinterpret_cast<LG_MATCH_START_PACKET*>(_packet);
			if (p->match_num < 0 || p->match_num >= MAX_MATCH) {
				LogPrinter::PrintMsg("Invalid lobby match index"); return;
			}
			m_timerQueue.push({ p->match_num, TimeUtil::PassedTimeMSec(1000), EVENT_TYPE::EV_CONNECT_UPDATE, -1 });
			break;
		}
		case LG_MATCH_PLAYER: {
			LG_MATCH_PACKET* p = reinterpret_cast<LG_MATCH_PACKET*>(_packet);
			if (p->match_num < 0 || p->match_num >= MAX_MATCH || p->id < 0 || p->id >= MAX_PLAYER) {
				LogPrinter::PrintMsg("Invalid lobby player index"); return;
			}
			CObjectMgr::GetInstance()->RegisterClientToServer(p->name, p->id, p->match_num, p->model);
			break;
		}
		default:
			LogPrinter::PrintMsg(static_cast<int>(_packet->type) + ": Undefined Packet From Lobby Server");
			break;
		}
	}

	void CNetworkMgr::InitializeMonster(int _matchNum)
	{
        // CSV가 제공하는 공통 위치 수 안에서만 그룹별 중복 없는 index를 고른다.
        const auto spawnGroup = [_matchNum](int _begin, int _count) {
            size_t positions = SIZE_MAX;
            for (int i=_begin; i<_begin+_count; ++i) {
                const auto npc = CObjectMgr::GetInstance()->GetNpc(_matchNum,i);
                const auto csv = NpcCsvMgr::GetInstance()->GetNpcCsv(npc->GetNpcType());
                if (!csv) throw std::runtime_error("missing NPC CSV for spawn");
                positions = (std::min)(positions,(std::min)(csv->m_respawnPos.size(),csv->m_respawnLook.size()));
            }
            if (positions < static_cast<size_t>(_count)) throw std::runtime_error("insufficient NPC spawn positions");
            auto indices = RandomUtil::GenerateUniqueRandomNumbers(0,static_cast<int>(positions)-1,_count);
            auto position = indices.begin();
            for (int i=_begin; i<_begin+_count; ++i,++position)
                CObjectMgr::GetInstance()->GetNpc(_matchNum,i)->Initialize(*position);
        };
        spawnGroup(MAX_MINION,1);
        spawnGroup(MAX_MINION+1,2);
        spawnGroup(MAX_MINION+3,6);
	}

	void CNetworkMgr::RegisterSkillEvent(const SKILL_EVENT& _ev)
	{
		m_skillTimer->PushEvent(_ev);
	}
}
