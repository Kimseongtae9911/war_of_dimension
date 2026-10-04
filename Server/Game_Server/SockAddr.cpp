#include "pch.h"
#include "SockAddr.h"

namespace wod_server {

	SockAddr::SockAddr()
	{
		memset(&m_sockaddr, 0, sizeof(m_sockaddr));
		GetSockAddr()->sin_family = AF_INET;
		SetIP_type(INADDR_ANY);
		GetSockAddr()->sin_port = 0;
	}

	SockAddr::SockAddr(unsigned int addr, unsigned short port)
	{
		memset(&m_sockaddr, 0, sizeof(m_sockaddr));
		GetSockAddr()->sin_family = AF_INET;
		SetIP_type(htonl(addr));
		GetSockAddr()->sin_port = htons(port);
	}

	SockAddr::SockAddr(const sockaddr& addr)
	{
		memset(&m_sockaddr, 0, sizeof(m_sockaddr));
		memcpy(&m_sockaddr, &addr, sizeof(sockaddr));
	}

	SockAddr::SockAddr(unsigned short port)
	{
		memset(&m_sockaddr, 0, sizeof(m_sockaddr));
		GetSockAddr()->sin_family = AF_INET;
		SetIP_type(INADDR_ANY);
		GetSockAddr()->sin_port = htons(port);
	}

}