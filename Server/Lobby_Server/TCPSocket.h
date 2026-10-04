#pragma once
#include "SockAddr.h"

namespace wod_server {
	class SockAddr;

	class Session {
	public:
		Session(bool sock=false);
		virtual ~Session() {}

		void Send(void* packet);
		void Recv();

		OverlapEx& GetOverEx() { return m_over; }
		const int GetRemainData() const { return m_remainData; }
		const SOCKET& GetSocket() const { return m_sock; }

		void SetRemainData(const int bytes) { m_remainData = bytes; }
		void SetSocket(const SOCKET& sock) { m_sock = sock; }

	protected:
		OverlapEx m_over;
		SOCKET m_sock;
		int m_remainData;
	};

	class TCPSocket : public Session
	{
	public:
		TCPSocket();
		virtual ~TCPSocket() {}

		void Accept(const SOCKET& socket);
		int Bind(const SockAddr& addr);
		int Listen(int backlog = SOMAXCONN);

		const HANDLE& GetHandle() const { return m_iocp; }
		const SOCKET& GetClientSocket() const { return m_clsock; }
		void SetClientSocket(const SOCKET& sock) { m_clsock = sock; }
	private:
		HANDLE m_iocp;
		SOCKET m_clsock;
	};

}