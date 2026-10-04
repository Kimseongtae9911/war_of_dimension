#include "stdafx.h"
#include "TCPSocket.h"
#include "OverlapEx.h"
#include "SockAddr.h"
#include "SocketUtil.h"

Session::Session(bool sock)
{
    m_over = new OverlapEx();
    if (sock) {
        m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
        if (INVALID_SOCKET == m_sock) {
            SocketUtil::PrintError("Session Create");
        }
        m_register_time = static_cast<unsigned int>(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
    }
}

void Session::Send(void* packet)
{
    OverlapEx* over = new OverlapEx(reinterpret_cast<char*>(packet));

    int byte = WSASend(m_sock, &over->GetWSA(), 1, 0, 0, &over->GetOver(), 0);
    if (byte < 0) {
        SocketUtil::PrintError("Send");
    }
}

void Session::Recv()
{
    DWORD recv_flag = 0;
    memset(&m_over->GetOver(), 0, sizeof(m_over->GetOver()));
    m_over->GetWSA().len = BUF_SIZE - m_remainData;
    m_over->GetWSA().buf = m_over->GetSendBuf() + m_remainData;
    int byte = WSARecv(m_sock, &m_over->GetWSA(), 1, 0, &recv_flag, &m_over->GetOver(), 0);
}

//==========================================================================================



TCPSocket::TCPSocket()
{
    m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    if (INVALID_SOCKET == m_sock) {
        SocketUtil::PrintError("Session Create");
    }
    m_iocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, 0);
    CreateIoCompletionPort(reinterpret_cast<HANDLE>(m_sock), m_iocp, 9999, 0);
    //m_clsock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
    m_over->SetOP(OP_TYPE::OP_ACCEPT);
}

void TCPSocket::Accept(const SOCKET& socket)
{
    m_clsock = socket;
    int size = sizeof(SOCKADDR_IN);
    AcceptEx(m_sock, m_clsock, m_over->GetSendBuf(), 0, size + 16, size + 16, 0, &m_over->GetOver());
}

int TCPSocket::Bind(const SockAddr& addr)
{
    int err = bind(m_sock, &addr.m_sockaddr, sizeof(addr.m_sockaddr));
    if (0 != err) {
        SocketUtil::PrintError("Bind");
        return SocketUtil::GetLastError();
    }
    return NO_ERROR;
}

int TCPSocket::Listen(int backlog)
{
    int err = listen(m_sock, backlog);
    if (err < 0) {
        SocketUtil::PrintError("Listen");
        return SocketUtil::GetLastError();
    }
    return NO_ERROR;
}

void TCPSocket::Connect()
{
    SockAddr addr(LOBBY_PORT);

    int err = WSAConnect(m_sock, reinterpret_cast<sockaddr*>(addr.GetSockAddr()), sizeof(sockaddr_in), NULL, NULL, NULL, NULL);

    if (err != 0) {
        SocketUtil::PrintError("Connect");
    }
}
