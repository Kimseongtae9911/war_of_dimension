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

namespace wod::core {
class SocketError : public std::runtime_error {
public:
    SocketError(const char* operation, int code);
    int Code() const { return code_; }
private:
    int code_;
};
class WinsockRuntime {
public:
    WinsockRuntime();
    ~WinsockRuntime();
    WinsockRuntime(const WinsockRuntime&) = delete;
    WinsockRuntime& operator=(const WinsockRuntime&) = delete;
};
class SockAddr {
public:
    SockAddr();
    explicit SockAddr(unsigned short port);
    SockAddr(unsigned int addr, unsigned short port);
    explicit SockAddr(const sockaddr& addr);
    const sockaddr* Native() const { return reinterpret_cast<const sockaddr*>(&address_); }
private:
    sockaddr_in address_{};
};
enum class IoOperation { Receive, Send, Accept, Disconnect, AppEvent };
class IoContext {
public:
    static constexpr size_t Capacity = 256;
    struct NativeRecord { OVERLAPPED over{}; IoContext* owner; };
    IoContext();
    virtual ~IoContext() = default;
    IoContext(const IoContext&) = delete;
    IoContext& operator=(const IoContext&) = delete;
    OVERLAPPED& GetOver() { return native_.over; }
    WSABUF& GetWSA() { return buffer_; }
    char* GetSendBuf() { return bytes_; }
    void ResetOver();
    void Reset();
    void CopyPacket(std::span<const char> packet);
    static IoContext* FromOver(OVERLAPPED* over);
    IoOperation operation = IoOperation::Receive;
    SOCKET socket = INVALID_SOCKET;
    SOCKET listener = INVALID_SOCKET;
    ULONG_PTR key = 0;
    std::atomic_bool pending = false;
    size_t offset = 0;
    size_t length = 0;
    DWORD error = 0;
    bool deferred = false;
private:
    NativeRecord native_;
    WSABUF buffer_{};
    char bytes_[Capacity]{};
};
// 다음 Poll 호출 또는 Completion 파괴까지 해당 연결의 callback 실행권을 보유한다.
struct Completion {
    IoContext* context = nullptr;
    ULONG_PTR key = 0;
    DWORD bytes = 0;
    DWORD error = 0;
    std::shared_ptr<std::recursive_mutex> dispatchMutex;
    std::unique_lock<std::recursive_mutex> dispatchLock;
    void Clear() {
        dispatchLock = {}; dispatchMutex.reset();
        context = nullptr; key = bytes = error = 0;
    }
};
struct NetworkStats { uint64_t pending, received, sent, errors, sockets; };
class IocpService {
public:
    using SendCall = std::function<int(SOCKET, WSABUF*, OVERLAPPED*)>;
    explicit IocpService(SendCall sendCall = {});
    ~IocpService();
    IocpService(const IocpService&) = delete;
    IocpService& operator=(const IocpService&) = delete;
    SOCKET CreateSocket();
    void Attach(SOCKET socket, ULONG_PTR key);
    void Connect(SOCKET socket, const std::string& ip, unsigned short port);
    void Bind(SOCKET socket, const SockAddr& address);
    void Listen(SOCKET socket, int backlog);
    bool Receive(SOCKET socket, IoContext& context);
    bool Send(SOCKET socket, IoContext& context);
    bool Accept(SOCKET listener, SOCKET socket, IoContext& context);
    bool Disconnect(SOCKET socket, IoContext& context);
    bool Post(ULONG_PTR key, IoContext& context);
    void Close(SOCKET socket);
    bool Poll(Completion& completion);
    void RequestStop(size_t workers);
    void Drain();
    void Finish();
    bool IsStopping() const { return stopping_.load(); }
    const HANDLE& Handle() const { return handle_; }
    NetworkStats Stats() const;
private:
    struct SocketState {
        SOCKET socket = INVALID_SOCKET;
        ULONG_PTR key = 0;
        size_t pending = 0;
        bool disconnecting = false;
        bool attached = false;
        std::shared_ptr<std::recursive_mutex> dispatch = std::make_shared<std::recursive_mutex>();
        IoContext* disconnected = nullptr;
        std::deque<IoContext*> sends;
    };
    bool Begin(SOCKET socket, IoContext& context, IoOperation op);
    void IssueSend(SocketState& state, IoContext& context);
    void Fail(IoContext& context, DWORD error);
    void Complete(IoContext& context);
    void Wake();
    SocketState& State(SOCKET socket);
    HANDLE handle_ = nullptr;
    mutable std::recursive_mutex mutex_;
    std::unordered_map<SOCKET, SocketState> sockets_;
    std::unordered_map<ULONG_PTR, std::shared_ptr<std::recursive_mutex>> appDispatch_;
    std::atomic_bool stopping_ = false;
    std::atomic_uint64_t pending_ = 0, received_ = 0, sent_ = 0, errors_ = 0;
    size_t workers_ = 1;
    SendCall sendCall_;
};

// 수신된 영역만 분리하며 출력 프레임이 패킷 bytes를 소유한다.
class FrameDecoder {
public:
    using Frame = std::vector<char>;
    static bool Extract(char* buffer, size_t bytes, size_t& remain, std::vector<Frame>& frames);
};
class ProcessStopSignal {
public:
    ProcessStopSignal();
    ~ProcessStopSignal();
    void Wait(std::chrono::milliseconds timeout = (std::chrono::milliseconds::max)());
    static bool Request(DWORD pid);
private:
    HANDLE event_ = nullptr;
};
}
