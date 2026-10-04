#include "pch.h"
#include "TCPSocket.h"
#include "SockAddr.h"
#include "SocketUtil.h"
#include "LogUtil.h"
#include "Resource.h"

namespace wod_server {

    Session::Session(bool sock)
    {
        if (sock) {
            m_sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
            if (INVALID_SOCKET == m_sock) {
                SocketUtil::PrintError("Session Create");
            }
            entryTime = TimeUtil::CurTime();
        }
        m_remainData = 0;
    }

    void Session::Connect(std::string ip)
    {
        SOCKADDR_IN server_addr;
        ZeroMemory(&server_addr, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(LOBBY_PORT);
        inet_pton(AF_INET, ip.c_str(), &server_addr.sin_addr);

        int ret = WSAConnect(m_sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr), NULL, NULL, NULL, NULL);
        if (SOCKET_ERROR == ret) {
            SocketUtil::PrintError("Connect Error");
            return;
        }
        LogPrinter::PrintMsg("Lobby Server Connected");
    }

    void Session::Send(void* packet)
    {       
        try {
            OverlapEx* over = Resource::GetOverObjectFromPool();
            over->Intialize(reinterpret_cast<BASE_PACKET*>(packet));
            WSASend(m_sock, &over->GetWSA(), 1, 0, 0, &over->GetOver(), 0);
        }
        catch (const std::exception& ex) {
            LogPrinter::PrintMsg("Err(TCPSocket Send) Packet Type: " + std::to_string(static_cast<int>(reinterpret_cast<BASE_PACKET*>(packet)->type - '0')) + std::string(ex.what()));
        }
    }

    void Session::Recv()
    {
        DWORD recv_flag = 0;
        memset(&m_over.GetOver(), 0, sizeof(m_over.GetOver()));
        m_over.GetWSA().len = BUF_SIZE - m_remainData;
        m_over.GetWSA().buf = m_over.GetSendBuf() + m_remainData;
        int byte = WSARecv(m_sock, &m_over.GetWSA(), 1, 0, &recv_flag, &m_over.GetOver(), 0);
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
       m_over.SetOP(OP_TYPE::OP_ACCEPT);
    }

    void TCPSocket::Accept(const std::shared_ptr<Session> session)
    {   
        m_clsock = session->GetSocket();
        m_over.SetSocketID(session->GetSocketID());
        int size = sizeof(SOCKADDR_IN);
        AcceptEx(m_sock, m_clsock, m_over.GetSendBuf(), 0, size + 16, size + 16, 0, &m_over.GetOver());
    }

    void TCPSocket::Bind(const SockAddr& addr)
    {
        int err = bind(m_sock, &addr.m_sockaddr, sizeof(addr.m_sockaddr));
        if (0 != err) {
            SocketUtil::PrintError("Bind");
        }
    }

    void TCPSocket::Listen(int backlog)
    {
        int err = listen(m_sock, backlog);
        if (err < 0) {
            SocketUtil::PrintError("Listen");
        }
    }
}