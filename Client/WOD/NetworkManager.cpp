#include "stdafx.h"
#include "NetworkManager.h"
#include "TCPSocket.h"
#include "SockAddr.h"
#include "OverlapEx.h"
#include "SocketUtil.h"

std::unique_ptr<NetworkManager> NetworkManager::m_instance;

void NetworkManager::Initialize()
{
	SocketUtil::Startup();
	m_handle = std::make_shared<TCPSocket>();
    
    m_iocpfunc.insert({ OP_TYPE::OP_RECV, [this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Recv(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_SEND, [this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Send(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_DISCONNECT,[this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Disconnect(id, bytes, over_ex); } });
    m_iocpfunc.insert({ OP_TYPE::OP_CONNECT,[this](int id, int bytes, OverlapEx* over_ex) {NetworkManager::Connect(id, bytes, over_ex); } });

    m_packetfunc.insert({ SC_LOGIN_INFO, [this](char* packet, int id) {NetworkManager::LoginInfoPacket(id, packet); } });
    m_packetfunc.insert({ SC_MOVE_PLAYER, [this](char* packet, int id) {NetworkManager::MovePacket(id, packet); } });
    m_packetfunc.insert({ SC_MATCH_PLAYER, [this](char* packet, int id) {NetworkManager::MatchPacket(id, packet); } });
    m_packetfunc.insert({ SC_MATCH_END, [this](char* packet, int id) {NetworkManager::MatchEndPacket(id, packet); } });

    GUID op = WSAID_DISCONNECTEX;
    DWORD bytes = 0;
    WSAIoctl(m_handle->GetSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER, &op, sizeof(op), &SocketUtil::DisconnectEx, sizeof(SocketUtil::DisconnectEx), &bytes, NULL, NULL);
    
    GUID op2 = WSAID_CONNECTEX;
    WSAIoctl(m_handle->GetSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER, &op2, sizeof(op2), &SocketUtil::ConnectEx, sizeof(SocketUtil::ConnectEx), &bytes, NULL, NULL);

    SockAddr claddr;
    m_handle->Bind(claddr);


    //WSAConnect(m_handle->GetSocket(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr), NULL, NULL, NULL, NULL);
    m_handle->Connect();
}

void NetworkManager::Release()
{
}

void NetworkManager::SendPacket(void* packet)
{
    m_handle->Send(packet);
}

void NetworkManager::RecvPacket()
{
    m_handle->Recv();    
}

void NetworkManager::WorkerThread()
{
    while (true) {
        DWORD bytes;
        ULONG_PTR key;
        WSAOVERLAPPED* over = nullptr;
        int err = GetQueuedCompletionStatus(m_handle->GetHandle(), &bytes, &key, &over, INFINITE);
        OverlapEx* over_ex = reinterpret_cast<OverlapEx*>(over);
        int id = static_cast<int>(key);

        m_iocpfunc[over_ex->GetOP()](static_cast<int>(key), bytes, over_ex);
    }
}

void NetworkManager::Recv(int id, int bytes, OverlapEx* over_ex)
{
    int remaindata = bytes + m_handle->GetRemainData();
    char* p = over_ex->GetSendBuf();
    while (remaindata > 0) {
        int p_size = p[0];
        if (p_size <= remaindata) {
            m_packetfunc[p[1]](p, id);
            p = p + p_size;
            remaindata = remaindata - p_size;
        }
        else break;
    }
    m_handle->SetRemainData(remaindata);
    if (remaindata > 0)
        memcpy(over_ex->GetSendBuf(), p, remaindata);
    RecvPacket();
}

void NetworkManager::Send(int id, int bytes, OverlapEx* over_ex)
{
    delete over_ex;
}

void NetworkManager::Disconnect(int id, int bytes, OverlapEx* over_ex)
{
    SOCKADDR_IN server_addr;
    ZeroMemory(&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(over_ex->GetInfo1());
    inet_pton(AF_INET, over_ex->GetInfo2(), &server_addr.sin_addr);

    OverlapEx* over = new OverlapEx();
    over->ResetOver();
    over->SetOP(OP_TYPE::OP_CONNECT);
    if (false == SocketUtil::ConnectEx(m_handle->GetSocket(), reinterpret_cast<LPSOCKADDR>(&server_addr), sizeof(server_addr), NULL, 0, NULL, &over->GetOver()) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect");
    }
    delete over_ex;
}

void NetworkManager::Connect(int id, int bytes, OverlapEx* over_ex)
{
    //g_lobby != g_lobby;

            ////Connect to Game Server
            //if (false == g_lobby) {
            //    CS_LOGIN_PACKET* packet = new CS_LOGIN_PACKET;
            //    packet->size = sizeof(CS_LOGIN_PACKET);
            //    packet->type = CS_LOGIN;
            //    memcpy_s(packet->name, 20, "name", 20);
            //    SendPacket(packet);
            //}
    delete over_ex;
}

void NetworkManager::LoginInfoPacket(int id, char* packet)
{
}

void NetworkManager::MovePacket(int id, char* packet)
{
}

void NetworkManager::MatchPacket(int id, char* packet)
{
    SC_MATCH_PACKET* p = reinterpret_cast<SC_MATCH_PACKET*>(packet);

    OverlapEx* over2 = new OverlapEx();
    over2->ResetOver();
    over2->SetOP(OP_TYPE::OP_DISCONNECT);
    over2->SetInfo1(p->gameport);
    over2->SetInfo2(p->gameip);
    if (false == SocketUtil::DisconnectEx(m_handle->GetSocket(), &over2->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect");
        delete over2;
    }
}

void NetworkManager::MatchEndPacket(int id, char* packet)
{
    SC_MATCH_END_PACKET* p = reinterpret_cast<SC_MATCH_END_PACKET*>(packet);

    OverlapEx* over2 = new OverlapEx();
    over2->ResetOver();
    over2->SetOP(OP_TYPE::OP_DISCONNECT);
    over2->SetInfo1(p->lobbyport);
    over2->SetInfo2(p->lobbyip);
    if (false == SocketUtil::DisconnectEx(m_handle->GetSocket(), &over2->GetOver(), TF_REUSE_SOCKET, NULL) && WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
        SocketUtil::PrintError("Disconnect");
        delete over2;
    }
    //g_match = false;
}


