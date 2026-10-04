#pragma once
	class SockAddr;
	class OverlapEx;

class Session {
public:
	Session(bool sock = false);
	virtual ~Session() {}

	void Send(void* packet);
	void Recv();

	OverlapEx* GetOverEx() { return m_over; }
	const int GetRemainData() const { return m_remainData; }
	const SOCKET& GetSocket() const { return m_sock; }

	void SetRemainData(const int bytes) { m_remainData = bytes; }
	void SetSocket(const SOCKET& sock) { m_sock = sock; }
	void SetRegisterTime() { m_register_time = static_cast<unsigned int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())); }
	constexpr bool operator < (const Session& L) const { return (m_register_time > L.m_register_time); }

protected:
	OverlapEx* m_over;
	SOCKET m_sock;
	int m_remainData;

private:
	unsigned int m_register_time;
};

class TCPSocket : public Session
{
public:
	TCPSocket();
	virtual ~TCPSocket() {}

	void Accept(const SOCKET& socket);
	int Bind(const SockAddr& addr);
	int Listen(int backlog = SOMAXCONN);
	void Connect();

	const HANDLE& GetHandle() const { return m_iocp; }
	const SOCKET& GetClientSocket() const { return m_clsock; }
	void SetClientSocket(const SOCKET& sock) { m_clsock = sock; }
private:
	HANDLE m_iocp;
	SOCKET m_clsock;
};