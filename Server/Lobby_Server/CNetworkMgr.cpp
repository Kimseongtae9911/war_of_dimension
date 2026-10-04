#include "pch.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "SocketUtil.h"
#include "TCPSocket.h"
#include "Resource.h"

namespace wod_server {
	std::unique_ptr<CNetworkMgr> CNetworkMgr::m_instance;

	bool CNetworkMgr::Initialize()
	{
#ifndef LOCAL_TEST
		std::cout << "Input Game Server IP: ";
		std::cin >> gameIP;
#else
		gameIP = "127.0.0.1";
#endif

#ifdef WITH_DATABASE
		m_dataBaseThread = new CDataBaseThread();
#endif
		m_handle = std::make_shared<TCPSocket>();
		m_gameServer = std::make_shared<Session>(true);

		m_iocpfunc.insert({ OP_TYPE::OP_SERVER_CONNECT, [this](int id, int bytes, OverlapEx* over_ex) {ServerConnect(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_ACCEPT, [this](int id, int bytes, OverlapEx* over_ex) {CNetworkMgr::Accept(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, OverlapEx* over_ex) {CNetworkMgr::Recv(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, OverlapEx* over_ex) {CNetworkMgr::Send(id, bytes, over_ex); } });
		m_iocpfunc.insert({ OP_TYPE::OP_DISCONNECT,[this](int id, int bytes, OverlapEx* over_ex) {CNetworkMgr::Disconnect(id, bytes, over_ex); } });		;

		// Make Socket Pool
		for (int i = 0; i < MAX_SOCKET; ++i) {
			SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			Resource::socketpool.push(s);

			//Register socket to iocp, Create Client Objects
			int id = static_cast<int>(s);
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), m_handle->GetHandle(), id, 0);
			CUserMgr::GetInstance()->MakeClientObject(id);
		}

		for (int i = 0; i < MAX_OVEREX_OBJECT; ++i) {
			OverlapEx* overEx = new OverlapEx;
			Resource::overExPool.push(overEx);
		}

		m_handle->Bind(SockAddr(LOBBY_PORT));
		m_handle->Listen();

		SocketResource socketResource;
		Resource::socketpool.try_pop(socketResource);
		m_handle->Accept(socketResource.socket);

		GUID op = WSAID_DISCONNECTEX;
		DWORD bytes = 0;
		WSAIoctl(m_handle->GetSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER, &op, sizeof(op), &SocketUtil::DisconnectEx, sizeof(SocketUtil::DisconnectEx), &bytes, NULL, NULL);

		m_clientNum = 0;
		m_gameseverConnected = false;

		m_p2pNetwork = new CP2PNetwork;

		return true;
	}

	bool CNetworkMgr::Release()
	{
		try {
#ifdef WITH_DATABASE
			delete m_dataBaseThread;
#endif
			delete m_p2pNetwork;
			return true;
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg("Err(NetworkMgr Release): " + std::string(ex.what()));
			return false;
		}
	}

	void CNetworkMgr::IOCPFunc()
	{
		while (true) {
			DWORD bytes;
			ULONG_PTR key;
			WSAOVERLAPPED* over = nullptr;
			int err = GetQueuedCompletionStatus(m_handle->GetHandle(), &bytes, &key, &over, INFINITE);
			OverlapEx* overEx = reinterpret_cast<OverlapEx*>(over);

			if (!m_gameseverConnected) {
				m_gameseverConnected = true;
				overEx->SetOP(OP_TYPE::OP_SERVER_CONNECT);
			}

			if (0 == err) {
				if (OP_TYPE::OP_ACCEPT == overEx->GetOP())
					LogPrinter::PrintMsg("Accept Error");
				else if (OP_TYPE::OP_SEND == overEx->GetOP()) {
					overEx->Reset();
					Resource::overExPool.push(overEx);
				}
				else
					CUserMgr::GetInstance()->DisconnectClient(static_cast<int>(key));
				continue;
			}

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
			auto current_time = std::chrono::system_clock::now();
			if (m_timerQueue.try_pop(ev)) {
				if (ev.wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::yield();
					continue;
				}
				switch (ev.eventID) {
				default:
					break;
				}
				continue;
			}
			std::this_thread::yield();
		}
	}

	void CNetworkMgr::DataBaseFunc()
	{
		m_dataBaseThread->ThreadFunction();
	}

	const HANDLE& CNetworkMgr::GetHandle() const
	{
		return m_handle->GetHandle();
	}

	void CNetworkMgr::ServerConnect(int id, int bytes, OverlapEx* over_ex)
	{
		m_gameServer->SetRemainData(0);
		m_gameServer->SetSocket(m_handle->GetClientSocket());

		CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_handle->GetClientSocket()), m_handle->GetHandle(), static_cast<int>(m_gameServer->GetSocket()), 0);
		m_gameServer->Recv();
		m_handle->GetOverEx().ResetOver();
		m_handle->GetOverEx().SetOP(OP_TYPE::OP_ACCEPT);
		SocketResource socketResource;
		Resource::socketpool.try_pop(socketResource);
		m_handle->Accept(socketResource.socket);
	}

	void CNetworkMgr::Accept(int id, int bytes, OverlapEx* over_ex)
	{
		if (m_clientNum >= MAX_CLIENT) {
			LogPrinter::PrintMsg("Max user exceeded");
		}
		else {
			LogPrinter::PrintMsg("Accept");

			CUserMgr::GetInstance()->InitializeClient(static_cast<int>(m_handle->GetClientSocket()));

			SOCKADDR_IN clientAddress;
			int clientAddressLength = sizeof(clientAddress);
			getpeername(m_handle->GetClientSocket(), reinterpret_cast<SOCKADDR*>(&clientAddress), &clientAddressLength);

			char clientIPAddress[INET_ADDRSTRLEN];
			inet_ntop(AF_INET, &(clientAddress.sin_addr), clientIPAddress, INET_ADDRSTRLEN);

			m_p2pNetwork->RegisterPeer(static_cast<int>(m_handle->GetClientSocket()), std::string(clientIPAddress));

			++m_clientNum;
		}
		m_handle->GetOverEx().ResetOver();

		SocketResource socketResource;
		if (Resource::socketpool.try_pop(socketResource))
			m_handle->Accept(socketResource.socket);
		else {
			SOCKET s = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
			CreateIoCompletionPort(reinterpret_cast<HANDLE>(s), m_handle->GetHandle(), static_cast<int>(s), 0);
			m_handle->Accept(s);
		}
	}

	void CNetworkMgr::Recv(int id, int bytes, OverlapEx* overEx)
	{
		if (id == m_gameServer->GetSocket()) {
			int remaindata = bytes + m_gameServer->GetRemainData();
			char* packet = overEx->GetSendBuf();

			while (remaindata > 0) {
				BASE_PACKET* p = reinterpret_cast<BASE_PACKET*>(packet);

				if (p->size <= remaindata) {
					PacketExec(p);
					packet += p->size;
					remaindata -= p->size;
				}
				else break;
			}
			m_gameServer->SetRemainData(remaindata);
			if (remaindata > 0)
				memmove(overEx->GetSendBuf(), packet, remaindata);
			m_gameServer->Recv();
		}
		else {
			CUserMgr::GetInstance()->GetClient(id)->RecvPacket(bytes, overEx);
		}
	}

	void CNetworkMgr::Send(int id, int bytes, OverlapEx* overEx)
	{
		overEx->Reset();
		Resource::overExPool.push(overEx);
	}

	void CNetworkMgr::Disconnect(int id, int bytes, OverlapEx* overEx)
	{
		CClient* client = CUserMgr::GetInstance()->GetClient(id);
		client->SetDisconnected();
#ifdef WITH_DATABASE
		if (strstr(client->GetName(), "Dummy") == nullptr) {
			m_dataBaseThread->RegisterDBEvent(DB_EVENT(client->GetName(), client->GetPlayerInfo(), 
				std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_INFO));
		}
#endif
		if(!client->IsInQueue())
			CUserMgr::GetInstance()->ClientReset(id);

		overEx->Reset();
		Resource::overExPool.push(overEx);

		--m_clientNum;
	}

	void CNetworkMgr::PacketExec(BASE_PACKET* packet)
	{
		//Game Server Packet Execution
		switch (packet->type) {
		case GL_TRANSACTIONS:
		{
			GL_TRANSACTIONS_PACKET* p = reinterpret_cast<GL_TRANSACTIONS_PACKET*>(packet);
			m_dataBaseThread->RegisterDBEvent(DB_EVENT(std::string(p->name), p->token, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));

#ifdef WITH_DATABASE
			TransactionData transaction;
			memcpy_s(transaction.name, NAME_SIZE, p->name, NAME_SIZE);
			transaction.token = p->token;
			memcpy_s(transaction.time, NAME_SIZE, p->time, NAME_SIZE);

			m_p2pNetwork->InsertTransaction(transaction);
#endif
		}
		break;
			
		}
	}
}