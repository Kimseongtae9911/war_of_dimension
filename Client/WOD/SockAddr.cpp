#include "stdafx.h"
#include "SockAddr.h"

SockAddr::SockAddr()
{
	memset(&m_sockaddr, 0, sizeof(m_sockaddr));
	GetSockAddr()->sin_family = AF_INET;
	GetSockAddr()->sin_port = 0;
	SetIP_type(INADDR_ANY);
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
	//SetIP_type(INADDR_ANY);
	GetSockAddr()->sin_port = htons(port);
	inet_pton(AF_INET, "127.0.0.1", &GetSockAddr()->sin_addr);
}