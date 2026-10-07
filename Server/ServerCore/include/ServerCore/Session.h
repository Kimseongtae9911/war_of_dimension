#pragma once
#include <ServerCore/Net.h>

namespace wod::core {
class TransportHost {
public:
    static void Start();
    static void Stop();
    static IocpService& Get();
};
template<class Context> class BasicSession {
public:
    using Acquire = std::function<Context*()>;
    using Return = std::function<void(Context*)>;
    BasicSession(IocpService& service, Acquire acquire, Return release, bool socket = false)
        : service_(service), acquire_(std::move(acquire)), release_(std::move(release)) {
        if (socket) m_sock = service_.CreateSocket();
    }
    virtual ~BasicSession() = default;
    void Send(void* packet) {
        if (!packet || m_sock == INVALID_SOCKET) return;
        auto* over = acquire_();
        try {
            const auto size = *static_cast<unsigned char*>(packet);
            over->CopyPacket(std::span(static_cast<const char*>(packet), size));
            over->SetTransportOperation(IoOperation::Send);
            if (!service_.Send(m_sock, *over)) release_(over);
        } catch (...) { release_(over); throw; }
    }
    void Recv() {
        if (service_.IsStopping() || m_sock == INVALID_SOCKET) return;
        if (m_remainData < 0 || m_remainData >= static_cast<int>(IoContext::Capacity)) throw std::logic_error("receive capacity");
        m_over.ResetOver(); m_over.SetTransportOperation(IoOperation::Receive);
        m_over.GetWSA().len = static_cast<ULONG>(IoContext::Capacity - m_remainData);
        m_over.GetWSA().buf = m_over.GetSendBuf() + m_remainData;
        service_.Receive(m_sock, m_over);
    }
    bool Decode(size_t bytes, Context& over, std::vector<FrameDecoder::Frame>& frames) {
        size_t remain = static_cast<size_t>(m_remainData);
        bool ok = FrameDecoder::Extract(over.GetSendBuf(), bytes, remain, frames);
        m_remainData = static_cast<int>(remain); return ok;
    }
    void Connect(const std::string& ip, unsigned short port) { service_.Connect(m_sock, ip, port); }
    void SetSocket(SOCKET socket) { std::lock_guard lock(generationMutex_); m_sock = socket; m_remainData = 0; ++generation_; }
    void Invalidate() { std::lock_guard lock(generationMutex_); ++generation_; m_remainData = 0; }
    template<class Func> void WithGeneration(uint64_t generation, Func&& callback) {
        std::lock_guard lock(generationMutex_);
        if (generation == generation_.load()) std::invoke(std::forward<Func>(callback));
    }
    uint64_t Generation() const { return generation_.load(); }
    const SOCKET& GetSocket() const { return m_sock; }
    Context& GetOverEx() { return m_over; }
    int GetRemainData() const { return m_remainData; }
    void SetRemainData(int bytes) { m_remainData = bytes; }
protected:
    IocpService& service_;
    Acquire acquire_;
    Return release_;
    Context m_over;
    SOCKET m_sock = INVALID_SOCKET;
    int m_remainData = 0;
    std::atomic_uint64_t generation_ = 0;
    std::recursive_mutex generationMutex_;
};
template<class Context> class BasicListener : public BasicSession<Context> {
public:
    using Base = BasicSession<Context>;
    BasicListener(IocpService& service, typename Base::Acquire acquire, typename Base::Return release)
        : Base(service, std::move(acquire), std::move(release), true) {
        service.Attach(this->m_sock, 9999);
        this->m_over.SetTransportOperation(IoOperation::Accept);
    }
    void Accept(SOCKET socket) {
        m_clsock = socket;
        this->m_over.Reset(); this->m_over.SetTransportOperation(IoOperation::Accept);
        // 서버별 cookie는 Reset 전후 보존하는 어댑터에서 지정한다.
        this->service_.Accept(this->m_sock, socket, this->m_over);
    }
    int Bind(const SockAddr& address) { this->service_.Bind(this->m_sock, address); return 0; }
    int Listen(int backlog = SOMAXCONN) { this->service_.Listen(this->m_sock, backlog); return 0; }
    const HANDLE& GetHandle() const { return this->service_.Handle(); }
    const SOCKET& GetClientSocket() const { return m_clsock; }
    void SetClientSocket(SOCKET socket) { m_clsock = socket; }
private:
    SOCKET m_clsock = INVALID_SOCKET;
};
}
