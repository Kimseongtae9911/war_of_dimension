#pragma once
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include "TCPSocket.h"
#include "CSkillTimer.h"

namespace wod_server {
	class Session;
	class CClient;
	struct SKILL_EVENT;

	enum class EVENT_TYPE { EV_CONNECT_UPDATE, EV_READY_UPDATE, EV_LOADING_UPDATE, EV_MATCH_UPDATE, EV_SKILL_END, EV_STAT_CHANGE, EV_MATCH_FINISH, EV_HEALTHMANA_CHANGE, EV_NPC_ACTIVE};
	struct TIMER_EVENT {
		int m_objID;
		std::chrono::system_clock::time_point m_wakeUpTime;
		EVENT_TYPE m_eventID;
		int m_targetID;
		CStat m_changeStat;
		int m_changeMaxHp;
		int m_changeMaxMp;
        uint64_t m_sessionGeneration = 0;
        bool m_checkSessionGeneration = false;
		constexpr bool operator < (const TIMER_EVENT& _l) const
		{
			return (m_wakeUpTime > _l.m_wakeUpTime);
		}
	};

	class CNetworkMgr : public TSingleton<CNetworkMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void IOCPFunc();
        void PrepareStop() { m_stopping.store(true); }
        bool WorkerFailed() const { return m_workerFailed.load(); }
        bool IsStopping() const { return m_stopping.load(); }
		void TimerFunc();

		const HANDLE& GetHandle() const { return m_handle->GetHandle(); }

		void RegisterTimerEvent(const TIMER_EVENT& _ev);
		void RegisterSkillEvent(const SKILL_EVENT& _ev);

		void InitializeMonster(int _matchNum);

		void SendPacketToLobby(BASE_PACKET* _pkt) { m_LobbyServer->Send(_pkt); }

	private:
		void ProcessTimerEvent(const TIMER_EVENT& _ev);
		//IOCP Func
		void Accept(int _id, int _bytes, OverlapEx* _over_ex);
		void Recv(int _id, int _bytes, OverlapEx* _over_ex);
		void Send(int _id, int _bytes, OverlapEx* _over_ex);
		void Disconnect(int _id, int _bytes, OverlapEx* _over_ex);
		void ConnectUpdate(int _id, int _bytes, OverlapEx* _over_ex);
		void ReadyUpdate(int _id, int _bytes, OverlapEx* _over_ex);
		void LoadingUpdate(int _id, int _bytes, OverlapEx* _over_ex);
		void MatchUpdate(int _id, int _bytes, OverlapEx* _overEx);
		void MonsterHeal(int _id, int _bytes, OverlapEx* _overEx);
		void NpcActive(int _id, int _bytes, OverlapEx* _overEx);
		void MatchFinish(int _id, int _bytes, OverlapEx* _overEx);
		void HealthManaChange(int _id, int _bytes, OverlapEx* _overEx);

		//Packet Func
		void Packet_Exec(BASE_PACKET* _packet);

	public:
		std::string m_lobbyIP;

	private:
		std::atomic_bool m_stopping = false;
        std::atomic_bool m_workerFailed = false;
        std::shared_ptr<TCPSocket> m_handle = nullptr;
		std::shared_ptr<Session> m_LobbyServer = nullptr;

		std::atomic<int> m_clientID;
		std::atomic<int> m_clientnum;

		std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;

		CSkillTimer* m_skillTimer = nullptr;
		concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;
	};
}
using network = wod_server::CNetworkMgr;

