#pragma once

namespace wod_server {

	class SockAddr
	{
	public:
		SockAddr();
		SockAddr(unsigned int addr, unsigned short port);
		SockAddr(const sockaddr& addr);
		SockAddr(unsigned short port);

	private:
		const unsigned int& GetIP4Ref() const { return *reinterpret_cast<const unsigned int*>(&GetSockAddr()->sin_addr.S_un.S_addr); }
		void SetIP_type(unsigned int type) { GetSockAddr()->sin_addr.S_un.S_addr = type; }

		sockaddr_in* GetSockAddr() { return reinterpret_cast<sockaddr_in*>(&m_sockaddr); }
		const sockaddr_in* GetSockAddr() const { return reinterpret_cast<const sockaddr_in*>(&m_sockaddr); }

	private:
		friend class TCPSocket;
		sockaddr m_sockaddr;
	};

}