#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

namespace wod::core
{
class SocketError : public std::runtime_error
{
  public:
    SocketError(const char *_operation, int _code);

    int Code() const
    {
        return m_code;
    }

  private:
    int m_code;
};

class WinsockRuntime
{
  public:
    WinsockRuntime();
    ~WinsockRuntime();
    WinsockRuntime(const WinsockRuntime &) = delete;
    WinsockRuntime &operator=(const WinsockRuntime &) = delete;
};

class SockAddr
{
  public:
    SockAddr();
    explicit SockAddr(unsigned short _port);
    SockAddr(unsigned int _addr, unsigned short _port);
    explicit SockAddr(const sockaddr &_addr);

    const sockaddr *Native() const
    {
        return reinterpret_cast<const sockaddr *>(&m_address);
    }

  private:
    sockaddr_in m_address{};
};
enum class IoOperation
{
    Receive,
    Send,
    Accept,
    Disconnect,
    AppEvent
};

class IoContext
{
  public:
    static constexpr size_t m_Capacity = 256;

    struct NativeRecord
    {
        OVERLAPPED m_over{};
        IoContext *m_owner;
    };

    IoContext();
    virtual ~IoContext() = default;
    IoContext(const IoContext &) = delete;
    IoContext &operator=(const IoContext &) = delete;

    OVERLAPPED &GetOver()
    {
        return m_native.m_over;
    }

    WSABUF &GetWSA()
    {
        return m_buffer;
    }

    char *GetSendBuf()
    {
        return m_bytes;
    }

    void ResetOver();
    void Reset();
    void CopyPacket(std::span<const char> _packet);
    static IoContext *FromOver(OVERLAPPED *_over);
    IoOperation m_operation = IoOperation::Receive;
    SOCKET m_socket = INVALID_SOCKET;
    SOCKET m_listener = INVALID_SOCKET;
    ULONG_PTR m_key = 0;
    std::atomic_bool m_pending = false;
    size_t m_offset = 0;
    size_t m_length = 0;
    DWORD m_error = 0;
    bool m_deferred = false;

  private:
    NativeRecord m_native;
    WSABUF m_buffer{};
    char m_bytes[m_Capacity]{};
};

// 다음 Poll 호출 또는 Completion 파괴까지 해당 연결의 callback 실행권을 보유한다.
struct Completion
{
    IoContext *m_context = nullptr;
    ULONG_PTR m_key = 0;
    DWORD m_bytes = 0;
    DWORD m_error = 0;
    std::shared_ptr<std::recursive_mutex> m_dispatchMutex;
    std::unique_lock<std::recursive_mutex> m_dispatchLock;

    void Clear()
    {
        m_dispatchLock = {};
        m_dispatchMutex.reset();
        m_context = nullptr;
        m_key = m_bytes = m_error = 0;
    }
};

struct NetworkStats
{
    uint64_t m_pending, m_received, m_sent, m_errors, m_sockets;
};

class IocpService
{
  public:
    using SendCall = std::function<int(SOCKET, WSABUF *, OVERLAPPED *)>;
    explicit IocpService(SendCall _sendCall = {});
    ~IocpService();
    IocpService(const IocpService &) = delete;
    IocpService &operator=(const IocpService &) = delete;
    SOCKET CreateSocket();
    void Attach(SOCKET _socket, ULONG_PTR _key);
    void Connect(SOCKET _socket, const std::string &_ip, unsigned short _port);
    void Bind(SOCKET _socket, const SockAddr &_address);
    void Listen(SOCKET _socket, int _backlog);
    bool Receive(SOCKET _socket, IoContext &_context);
    bool Send(SOCKET _socket, IoContext &_context);
    bool Accept(SOCKET _listener, SOCKET _socket, IoContext &_context);
    bool Disconnect(SOCKET _socket, IoContext &_context);
    bool Post(ULONG_PTR _key, IoContext &_context);
    void Close(SOCKET _socket);
    bool Poll(Completion &_completion);
    void RequestStop(size_t _workers);
    void Drain();
    void Finish();

    bool IsStopping() const
    {
        return m_stopping.load();
    }

    const HANDLE &Handle() const
    {
        return m_handle;
    }

    NetworkStats Stats() const;

  private:
    struct SocketState
    {
        SOCKET m_socket = INVALID_SOCKET;
        ULONG_PTR m_key = 0;
        size_t m_pending = 0;
        bool m_disconnecting = false;
        bool m_attached = false;
        std::shared_ptr<std::recursive_mutex> m_dispatch = std::make_shared<std::recursive_mutex>();
        IoContext *m_disconnected = nullptr;
        std::deque<IoContext *> m_sends;
    };

    bool Begin(SOCKET _socket, IoContext &_context, IoOperation _op);
    void IssueSend(SocketState &_state, IoContext &_context);
    void Fail(IoContext &_context, DWORD _error);
    void Complete(IoContext &_context);
    void Wake();
    SocketState &State(SOCKET _socket);
    HANDLE m_handle = nullptr;
    mutable std::recursive_mutex m_mutex;
    std::unordered_map<SOCKET, SocketState> m_sockets;
    std::unordered_map<ULONG_PTR, std::shared_ptr<std::recursive_mutex>> m_appDispatch;
    std::atomic_bool m_stopping = false;
    std::atomic_uint64_t m_pending = 0, m_received = 0, m_sent = 0, m_errors = 0;
    size_t m_workers = 1;
    SendCall m_sendCall;
};

// 수신된 영역만 분리하며 출력 프레임이 패킷 bytes를 소유한다.
class FrameDecoder
{
  public:
    using Frame = std::vector<char>;
    static bool Extract(char *_buffer, size_t _bytes, size_t &_remain, std::vector<Frame> &_frames);
};

class ProcessStopSignal
{
  public:
    ProcessStopSignal();
    ~ProcessStopSignal();
    void Wait(std::chrono::milliseconds _timeout = (std::chrono::milliseconds::max)());
    static bool Request(DWORD _pid);

  private:
    HANDLE m_event = nullptr;
};
}
