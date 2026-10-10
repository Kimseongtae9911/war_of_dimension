#pragma once
#include <ServerCore/Net.h>
namespace wod_server {
class OverlapEx final : public wod::core::IoContext {
public:
    void Intialize(BASE_PACKET* _packet) { CopyPacket(std::span(reinterpret_cast<const char*>(_packet), _packet->size)); SetOP(OP_TYPE::OP_SEND); }
    void Reset() { wod::core::IoContext::Reset(); m_op = OP_TYPE::OP_RECV; }
    OP_TYPE GetOP() const { return m_op; }
    void SetOP(OP_TYPE _op) { m_op = _op; }
    void SetTransportOperation(wod::core::IoOperation _op) {
        switch (_op) {
        case wod::core::IoOperation::Receive: m_op = OP_TYPE::OP_RECV; break;
        case wod::core::IoOperation::Send: m_op = OP_TYPE::OP_SEND; break;
        case wod::core::IoOperation::Accept: m_op = OP_TYPE::OP_ACCEPT; break;
        case wod::core::IoOperation::Disconnect: m_op = OP_TYPE::OP_DISCONNECT; break;
        default: break;
        }
    }
    int GetSocketID() const { return m_socketid; }
    int GetInfo() const { return m_info; }
    void SetSocketID(int _id) { m_socketid = _id; }
    void SetInfo(int _info) { m_info = _info; }
private:
    OP_TYPE m_op = OP_TYPE::OP_RECV;
    int m_socketid = -1;
    int m_info = 0;
};
}
