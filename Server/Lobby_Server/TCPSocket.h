#pragma once
#include "SockAddr.h"
#include "SocketUtil.h"
#include "Resource.h"
#include <ServerCore/Session.h>
namespace wod_server {
class Session : public wod::core::BasicSession<OverlapEx> {
public:
    explicit Session(bool socket = false) : BasicSession(SocketUtil::Runtime(),
        [] { return Resource::GetOverObjectFromPool(); },
        [](OverlapEx* value) { Resource::overExPool.push(value); }, socket) {}
    void Connect(const std::string& ip) { BasicSession::Connect(ip, LOBBY_PORT); }
    int GetSocketID() const { return m_socketid; }
    void SetSocketID(int id) { m_socketid = id; }
    std::chrono::system_clock::time_point entryTime = std::chrono::system_clock::now();
private:
    int m_socketid = -1;
};
class TCPSocket : public wod::core::BasicListener<OverlapEx> {
public:
    TCPSocket() : BasicListener(SocketUtil::Runtime(),
        [] { return Resource::GetOverObjectFromPool(); },
        [](OverlapEx* value) { Resource::overExPool.push(value); }) {}
    using BasicListener::Accept;
    void Accept(const std::shared_ptr<Session>& session) {
        GetOverEx().SetSocketID(session->GetSocketID()); BasicListener::Accept(session->GetSocket());
    }
};
}
