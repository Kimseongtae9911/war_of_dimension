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
		int m_objID = -1;
		std::chrono::system_clock::time_point m_wakeUpTime = {};
		EVENT_TYPE m_eventID = EVENT_TYPE::None;
		int m_targetID = -1;
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
		void DataBaseFunc();

		const HANDLE& GetHandle() const;
		CDataBaseThread* GetDataBaseThread() const { return m_dataBaseThread; }
		CP2PNetwork* GetP2PNetwork() const { return m_p2pNetwork; }

		void RegisterEvent(const TIMER_EVENT& _ev) { m_timerQueue.push(_ev); }

		const std::shared_ptr<Session> GetGameServer() const { return m_gameServer; }

		std::string m_gameIP;

	private:
		//IOCP Func
		void ServerConnect(int _id, int _bytes, OverlapEx* _over_ex);
		void Accept(int _id, int _bytes, OverlapEx* _over_ex);
		void Recv(int _id, int _bytes, OverlapEx* _over_ex);
		void Send(int _id, int _bytes, OverlapEx* _over_ex);
		void Disconnect(int _id, int _bytes, OverlapEx* _over_ex);

		void PacketExec(BASE_PACKET* _packet);

	private:
		std::atomic_bool m_stopping = false;
        std::atomic_bool m_workerFailed = false;
        std::shared_ptr<TCPSocket> m_handle = nullptr;
		std::shared_ptr<Session> m_gameServer = nullptr;
		bool m_gameseverConnected = false;
		std::atomic<int> m_clientNum = 0;

		concurrency::concurrent_priority_queue<TIMER_EVENT> m_timerQueue;
		std::unordered_map<OP_TYPE, std::function<void(int, int, OverlapEx*)>> m_iocpfunc;

		CDataBaseThread* m_dataBaseThread = nullptr;
		CP2PNetwork* m_p2pNetwork = nullptr;
	};
}
using network = wod_server::CNetworkMgr;
