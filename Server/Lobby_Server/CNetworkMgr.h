#pragma once

#include "CDataBaseThread.h"
#include "CP2PNetwork.h"

namespace wod_server {
	class TCPSocket;
	class Session;
	class CMoveObject;
	class CGameObject;
	class CClient;

	enum class EVENT_TYPE { None };
	struct TIMER_EVENT {
		int objID = -1;
		std::chrono::system_clock::time_point wakeUpTime = {};
		EVENT_TYPE eventID = EVENT_TYPE::None;
		int targetID = -1;
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
		void DataBaseFunc();

		const HANDLE& GetHandle() const;
		CDataBaseThread* GetDataBaseThread() const { return m_dataBaseThread; }
		CP2PNetwork* GetP2PNetwork() const { return m_p2pNetwork; }

		void RegisterEvent(const TIMER_EVENT& ev) { m_timerQueue.push(ev); }		

		const std::shared_ptr<Session> GetGameServer() const { return m_gameServer; }

		std::string gameIP;

	private:
		//IOCP Func
		void ServerConnect(int id, int bytes, OverlapEx* over_ex);
		void Accept(int id, int bytes, OverlapEx* over_ex);
		void Recv(int id, int bytes, OverlapEx* over_ex);
		void Send(int id, int bytes, OverlapEx* over_ex);
		void Disconnect(int id, int bytes, OverlapEx* over_ex);

		void PacketExec(BASE_PACKET* packet);

	private:
		std::shared_ptr<TCPSocket> m_handle = nullptr;
		std::shared_ptr<Session> m_gameServer = nullptr;
		bool m_gameseverConnected = false;
		std::atomic<int> m_clientNum = 0;

		concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;
		std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;		

		CDataBaseThread* m_dataBaseThread;		
		CP2PNetwork* m_p2pNetwork;
	};
}
using network = wod_server::CNetworkMgr;