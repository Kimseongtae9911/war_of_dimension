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
		int objID;
		std::chrono::system_clock::time_point wakeUpTime;
		EVENT_TYPE eventID;
		int targetID;
		CStat changeStat;
		int changeMaxHp;
		int changeMaxMp;
		constexpr bool operator < (const TIMER_EVENT& L) const
		{
			return (wakeUpTime > L.wakeUpTime);
		}
	};

	class CNetworkMgr : public TSingleton<CNetworkMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void IOCPFunc();
		void TimerFunc();

		const HANDLE& GetHandle() const { return m_handle->GetHandle(); }

		void RegisterTimerEvent(const TIMER_EVENT& ev) { m_timerQueue.push(ev); }
		void RegisterSkillEvent(const SKILL_EVENT& ev);		

		void InitializeMonster(int matchNum);

		void SendPacketToLobby(BASE_PACKET* pkt) { m_LobbyServer->Send(pkt); }

	private:
		//IOCP Func
		void Accept(int id, int bytes, OverlapEx* over_ex);
		void Recv(int id, int bytes, OverlapEx* over_ex);
		void Send(int id, int bytes, OverlapEx* over_ex);
		void Disconnect(int id, int bytes, OverlapEx* over_ex);
		void ConnectUpdate(int id, int bytes, OverlapEx* over_ex);
		void ReadyUpdate(int id, int bytes, OverlapEx* over_ex);
		void LoadingUpdate(int id, int bytes, OverlapEx* over_ex);
		void MatchUpdate(int id, int bytes, OverlapEx* overEx);		
		void MonsterHeal(int id, int bytes, OverlapEx* overEx);
		void NpcActive(int id, int bytes, OverlapEx* overEx);		
		void MatchFinish(int id, int bytes, OverlapEx* overEx);
		void HealthManaChange(int id, int bytes, OverlapEx* overEx);

		//Packet Func
		void Packet_Exec(BASE_PACKET* packet);		

	public:
		std::string lobbyIP;

	private:
		std::shared_ptr<TCPSocket> m_handle = nullptr;
		std::shared_ptr<Session> m_LobbyServer = nullptr;

		std::atomic<int> m_clientID;
		std::atomic<int> m_clientnum;
		
		std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;

		CSkillTimer* m_skillTimer;
		concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;
	};
}
using network = wod_server::CNetworkMgr;

