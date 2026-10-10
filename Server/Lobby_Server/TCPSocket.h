#pragma once
#include "SockAddr.h"
#include "SocketUtil.h"
#include "Resource.h"
#include <ServerCore/Session.h>
namespace wod_server {
class Session : public wod::core::BasicSession<OverlapEx> {
public:
    explicit Session(bool _socket = false) : BasicSession(SocketUtil::Runtime(),
        [] { return Resource::GetOverObjectFromPool(); },
        [](OverlapEx* _value) { Resource::m_overExPool.push(_value); }, _socket) {}
    void Connect(const std::string& _ip) { BasicSession::Connect(_ip, LOBBY_PORT); }
    int GetSocketID() const { return m_socketid; }
    void SetSocketID(int _id) { m_socketid = _id; }
    std::chrono::system_clock::time_point m_entryTime = std::chrono::system_clock::now();
private:
    int m_socketid = -1;
};
class TCPSocket : public wod::core::BasicListener<OverlapEx> {
public:
    TCPSocket() : BasicListener(SocketUtil::Runtime(),
        [] { return Resource::GetOverObjectFromPool(); },
        [](OverlapEx* _value) { Resource::m_overExPool.push(_value); }) {}
    using BasicListener::Accept;
    void Accept(const std::shared_ptr<Session>& _session) {
        GetOverEx().SetSocketID(_session->GetSocketID()); BasicListener::Accept(_session->GetSocket());
    }
};
}
