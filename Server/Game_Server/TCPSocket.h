#pragma once

namespace wod_server {
	class SockAddr;
	
	class Session {
	public:
		Session(bool sock=false);
		virtual ~Session() {}

		void Connect(std::string ip);

		void Send(void* packet);
		void Recv();		

		OverlapEx& GetOverEx() { return m_over; }
		const int GetRemainData() const { return m_remainData; }
		const SOCKET& GetSocket() const { return m_sock; }
		const int GetSocketID() const { return m_socketid; }

		void SetRemainData(const int bytes) { m_remainData = bytes; }
		void SetSocket(const SOCKET& sock) { m_sock = sock; }
		void SetSocketID(const int id) { m_socketid = id; }

		bool operator<(const Session& other) const {
			return entryTime > other.entryTime;
		}

		std::chrono::system_clock::time_point entryTime;
	protected:
		OverlapEx m_over;
		int m_remainData;
		SOCKET m_sock;
		int m_socketid;		
	};

	class TCPSocket : public Session
	{
	public:
		TCPSocket();
		virtual ~TCPSocket() {}

		void Accept(const std::shared_ptr<Session> session);
		void Bind(const SockAddr& addr);
		void Listen(int backlog = SOMAXCONN);

		void SetClientSocket(const SOCKET& socket) { m_clsock = socket; }

		const HANDLE& GetHandle() const { return m_iocp; }
		const SOCKET& GetClientSocket() const { return m_clsock; }

	private:
		HANDLE m_iocp;
		SOCKET m_clsock;
	};

}