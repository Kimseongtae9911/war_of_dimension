#pragma once
#include <ServerCore/Net.h>

namespace wod::core
{
class TransportHost
{
  public:
    static void Start();
    static void Stop();
    static IocpService &Get();
};

template <class Context> class BasicSession
{
  public:
    using Acquire = std::function<Context *()>;
    using Return = std::function<void(Context *)>;

    BasicSession(IocpService &_service, Acquire _acquire, Return _release, bool _socket = false) : m_service(_service), m_acquire(std::move(_acquire)), m_release(std::move(_release))
    {
        if (_socket)
            m_sock = m_service.CreateSocket();
    }

    virtual ~BasicSession() = default;

    void Send(void *_packet)
    {
        if (!_packet || m_sock == INVALID_SOCKET)
            return;

        auto *over = m_acquire();
        try
        {
            const auto size = *static_cast<unsigned char *>(_packet);
            over->CopyPacket(std::span(static_cast<const char *>(_packet), size));
            over->SetTransportOperation(IoOperation::Send);
            if (!m_service.Send(m_sock, *over))
                m_release(over);
        }
        catch (...)
        {
            m_release(over);
            throw;
        }
    }

    void Recv()
    {
        if (m_service.IsStopping() || m_sock == INVALID_SOCKET)
            return;

        if (m_remainData < 0 || m_remainData >= static_cast<int>(IoContext::m_Capacity))
            throw std::logic_error("receive capacity");

        m_over.ResetOver();
        m_over.SetTransportOperation(IoOperation::Receive);
        m_over.GetWSA().len = static_cast<ULONG>(IoContext::m_Capacity - m_remainData);
        m_over.GetWSA().buf = m_over.GetSendBuf() + m_remainData;
        m_service.Receive(m_sock, m_over);
    }

    bool Decode(size_t _bytes, Context &_over, std::vector<FrameDecoder::Frame> &_frames)
    {
        size_t remain = static_cast<size_t>(m_remainData);
        bool ok = FrameDecoder::Extract(_over.GetSendBuf(), _bytes, remain, _frames);
        m_remainData = static_cast<int>(remain);

        return ok;
    }

    void Connect(const std::string &_ip, unsigned short _port)
    {
        m_service.Connect(m_sock, _ip, _port);
    }

    void SetSocket(SOCKET _socket)
    {
        std::lock_guard lock(m_generationMutex);
        m_sock = _socket;
        m_remainData = 0;
        ++m_generation;
    }

    void Invalidate()
    {
        std::lock_guard lock(m_generationMutex);
        ++m_generation;
        m_remainData = 0;
    }

    template <class Func> void WithGeneration(uint64_t _generation, Func &&_callback)
    {
        std::lock_guard lock(m_generationMutex);
        if (_generation == m_generation.load())
            std::invoke(std::forward<Func>(_callback));
    }

    uint64_t Generation() const
    {
        return m_generation.load();
    }

    const SOCKET &GetSocket() const
    {
        return m_sock;
    }

    Context &GetOverEx()
    {
        return m_over;
    }

    int GetRemainData() const
    {
        return m_remainData;
    }

    void SetRemainData(int _bytes)
    {
        m_remainData = _bytes;
    }

  protected:
    IocpService &m_service;
    Acquire m_acquire;
    Return m_release;
    Context m_over;
    SOCKET m_sock = INVALID_SOCKET;
    int m_remainData = 0;
    std::atomic_uint64_t m_generation = 0;
    std::recursive_mutex m_generationMutex;
};

template <class Context> class BasicListener : public BasicSession<Context>
{
  public:
    using Base = BasicSession<Context>;

    BasicListener(IocpService &_service, typename Base::Acquire _acquire, typename Base::Return _release) : Base(_service, std::move(_acquire), std::move(_release), true)
    {
        _service.Attach(this->m_sock, 9999);
        this->m_over.SetTransportOperation(IoOperation::Accept);
    }

    void Accept(SOCKET _socket)
    {
        m_clsock = _socket;
        this->m_over.Reset();
        this->m_over.SetTransportOperation(IoOperation::Accept);
        // 서버별 cookie는 Reset 전후 보존하는 어댑터에서 지정한다.
        this->m_service.Accept(this->m_sock, _socket, this->m_over);
    }

    int Bind(const SockAddr &_address)
    {
        this->m_service.Bind(this->m_sock, _address);

        return 0;
    }

    int Listen(int _backlog = SOMAXCONN)
    {
        this->m_service.Listen(this->m_sock, _backlog);

        return 0;
    }

    const HANDLE &GetHandle() const
    {
        return this->m_service.Handle();
    }

    const SOCKET &GetClientSocket() const
    {
        return m_clsock;
    }

    void SetClientSocket(SOCKET _socket)
    {
        m_clsock = _socket;
    }

  private:
    SOCKET m_clsock = INVALID_SOCKET;
};
}
