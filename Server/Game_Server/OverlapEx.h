#pragma once
#include <ServerCore/Net.h>
namespace wod_server {
class OverlapEx final : public wod::core::IoContext {
public:
    void Intialize(BASE_PACKET* packet) { CopyPacket(std::span(reinterpret_cast<const char*>(packet), packet->size)); SetOP(OP_TYPE::OP_SEND); }
    void Reset() { wod::core::IoContext::Reset(); m_op = OP_TYPE::OP_RECV; m_hasGeneration = false; }
    OP_TYPE GetOP() const { return m_op; }
    void SetOP(OP_TYPE op) { m_op = op; }
    void SetTransportOperation(wod::core::IoOperation op) {
        switch (op) {
        case wod::core::IoOperation::Receive: m_op = OP_TYPE::OP_RECV; break;
        case wod::core::IoOperation::Send: m_op = OP_TYPE::OP_SEND; break;
        case wod::core::IoOperation::Accept: m_op = OP_TYPE::OP_ACCEPT; break;
        case wod::core::IoOperation::Disconnect: m_op = OP_TYPE::OP_DISCONNECT; break;
        default: break;
        }
    }
    void SetSessionGeneration(uint64_t generation) { m_generation = generation; m_hasGeneration = true; }
    bool HasSessionGeneration() const { return m_hasGeneration; }
    uint64_t GetSessionGeneration() const { return m_generation; }
    int GetSocketID() const { return m_socketid; }
    int GetInfo() const { return m_info; }
    void SetSocketID(int id) { m_socketid = id; }
    void SetInfo(int info) { m_info = info; }
private:
    OP_TYPE m_op = OP_TYPE::OP_RECV;
    int m_socketid = -1;
    int m_info = 0;
    uint64_t m_generation = 0;
    bool m_hasGeneration = false;
};
}
