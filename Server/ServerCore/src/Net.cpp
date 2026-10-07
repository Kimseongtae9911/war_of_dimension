#include <ServerCore/Net.h>
#include <ServerCore/Diagnostics.h>
#include <ServerCore/Session.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace wod::core {
namespace {
std::unique_ptr<WinsockRuntime> hostWinsock;
std::unique_ptr<IocpService> hostTransport;
}
void TransportHost::Start() {
    if (hostTransport) return;
    hostWinsock = std::make_unique<WinsockRuntime>();
    try { hostTransport = std::make_unique<IocpService>(); }
    catch (...) { hostWinsock.reset(); throw; }
}
void TransportHost::Stop() { hostTransport.reset(); hostWinsock.reset(); }
IocpService& TransportHost::Get() {
    if (!hostTransport) throw std::logic_error("transport not started");
    return *hostTransport;
}
SocketError::SocketError(const char* operation, int code)
    : std::runtime_error(std::string(operation) + ": " + std::to_string(code)), code_(code) {}
WinsockRuntime::WinsockRuntime() {
    WSADATA data{};
    const int error = WSAStartup(MAKEWORD(2, 2), &data);
    if (error) throw SocketError("WSAStartup", error);
}
WinsockRuntime::~WinsockRuntime() { WSACleanup(); }
SockAddr::SockAddr() : SockAddr(0) {}
SockAddr::SockAddr(unsigned short port) : SockAddr(INADDR_ANY, port) {}
SockAddr::SockAddr(unsigned int addr, unsigned short port) {
    address_.sin_family = AF_INET; address_.sin_addr.s_addr = htonl(addr); address_.sin_port = htons(port);
}
SockAddr::SockAddr(const sockaddr& addr) { std::memcpy(&address_, &addr, sizeof(address_)); }
IoContext::IoContext() : native_{{}, this} { Reset(); }
IoContext* IoContext::FromOver(OVERLAPPED* over) {
    static_assert(offsetof(NativeRecord, over) == 0);
    return over ? reinterpret_cast<NativeRecord*>(over)->owner : nullptr;
}
void IoContext::ResetOver() {
    if (pending.load()) throw std::logic_error("reset pending OVERLAPPED");
    native_.over = {};
}
void IoContext::Reset() {
    ResetOver(); buffer_.buf = bytes_; buffer_.len = static_cast<ULONG>(Capacity);
    operation = IoOperation::Receive; socket = INVALID_SOCKET; key = 0; offset = length = 0;
    error = 0; deferred = false; listener = INVALID_SOCKET;
}
void IoContext::CopyPacket(std::span<const char> packet) {
    if (packet.size() < 2 || packet.size() > 255 || static_cast<unsigned char>(packet[0]) != packet.size())
        throw std::invalid_argument("invalid send frame size");
    Reset(); length = packet.size(); buffer_.len = static_cast<ULONG>(length);
    std::memcpy(bytes_, packet.data(), length);
}
IocpService::IocpService(SendCall sendCall) : sendCall_(std::move(sendCall)) {
    handle_ = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!handle_) throw SocketError("CreateIoCompletionPort", GetLastError());
    if (!sendCall_) sendCall_ = [](SOCKET socket, WSABUF* buffer, OVERLAPPED* over) {
        return WSASend(socket, buffer, 1, nullptr, 0, over, nullptr);
    };
}
IocpService::~IocpService() {
    RequestStop(1); Drain(); Finish();
    if (handle_) CloseHandle(handle_);
}
SOCKET IocpService::CreateSocket() {
    std::lock_guard lock(mutex_);
    if (stopping_) throw std::logic_error("socket after stop");
    SOCKET socket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET) throw SocketError("WSASocket", WSAGetLastError());
    if (auto old = sockets_.find(socket); old != sockets_.end()) {
        if (old->second.pending) { closesocket(socket); throw std::logic_error("socket handle reused before drain"); }
        sockets_.erase(old);
    }
    sockets_.try_emplace(socket); sockets_.at(socket).socket = socket;
    return socket;
}
IocpService::SocketState& IocpService::State(SOCKET socket) {
    auto found = sockets_.find(socket);
    if (found == sockets_.end()) throw std::logic_error("unowned socket");
    return found->second;
}
void IocpService::Attach(SOCKET socket, ULONG_PTR key) {
    std::lock_guard lock(mutex_);
    auto& state = State(socket);
    if (state.attached) {
        if (state.key != key) throw std::logic_error("IOCP completion key is immutable");
        return;
    }
    if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(socket), handle_, key, 0))
        throw SocketError("attach IOCP", GetLastError());
    state.key = key; state.attached = true;
}
void IocpService::Connect(SOCKET socket, const std::string& ip, unsigned short port) {
    sockaddr_in address{}; address.sin_family = AF_INET; address.sin_port = htons(port);
    if (inet_pton(AF_INET, ip.c_str(), &address.sin_addr) != 1) throw SocketError("IPv4 address", WSAEINVAL);
    if (WSAConnect(socket, reinterpret_cast<sockaddr*>(&address), sizeof(address), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR)
        throw SocketError("WSAConnect", WSAGetLastError());
}
void IocpService::Bind(SOCKET socket, const SockAddr& address) {
    if (bind(socket, address.Native(), sizeof(sockaddr_in)) == SOCKET_ERROR) throw SocketError("bind", WSAGetLastError());
}
void IocpService::Listen(SOCKET socket, int backlog) {
    if (listen(socket, backlog) == SOCKET_ERROR) throw SocketError("listen", WSAGetLastError());
}
bool IocpService::Begin(SOCKET socket, IoContext& context, IoOperation op) {
    auto& state = State(socket);
    if (stopping_ || state.socket == INVALID_SOCKET || context.pending.exchange(true)) return false;
    context.operation = op; context.socket = socket; context.key = state.key;
    context.error = 0; context.offset = 0; context.deferred = false; context.GetOver() = {};
    ++state.pending; ++pending_; return true;
}
void IocpService::Fail(IoContext& context, DWORD error) {
    context.error = error;
    if (!PostQueuedCompletionStatus(handle_, 0, context.key, &context.GetOver()))
        throw SocketError("post failed I/O", GetLastError());
}
bool IocpService::Receive(SOCKET socket, IoContext& context) {
    std::lock_guard lock(mutex_);
    if (State(socket).disconnecting || !Begin(socket, context, IoOperation::Receive)) return false;
    DWORD flags = 0;
    if (WSARecv(socket, &context.GetWSA(), 1, nullptr, &flags, &context.GetOver(), nullptr) == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) Fail(context, error);
    }

    return true;
}
void IocpService::IssueSend(SocketState& state, IoContext& context) {
    context.GetOver() = {};
    context.GetWSA().buf = context.GetSendBuf() + context.offset;
    context.GetWSA().len = static_cast<ULONG>(context.length - context.offset);
    if (sendCall_(state.socket, &context.GetWSA(), &context.GetOver()) == SOCKET_ERROR) {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) Fail(context, error);
    }
}
bool IocpService::Send(SOCKET socket, IoContext& context) {
    std::lock_guard lock(mutex_);
    auto& state = State(socket);
    if (state.disconnecting || !Begin(socket, context, IoOperation::Send)) return false;
    state.sends.push_back(&context);
    if (state.sends.size() == 1) IssueSend(state, context);
    return true;
}
bool IocpService::Accept(SOCKET listener, SOCKET socket, IoContext& context) {
    std::lock_guard lock(mutex_);
    // AcceptEx 완료 key는 listener의 key다. 피연결 socket은 별도로 소유한다.
    if (!Begin(listener, context, IoOperation::Accept)) return false;
    context.listener = socket;
    const DWORD size = sizeof(sockaddr_in) + 16;
    if (!AcceptEx(listener, socket, context.GetSendBuf(), 0, size, size, nullptr, &context.GetOver())) {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) Fail(context, error);
    }

    return true;
}
bool IocpService::Disconnect(SOCKET socket, IoContext& context) {
    std::lock_guard lock(mutex_);
    auto& state = State(socket);
    if (state.disconnecting || !Begin(socket, context, IoOperation::Disconnect)) return false;
    state.disconnecting = true;
    CancelIoEx(reinterpret_cast<HANDLE>(socket), nullptr);
    while (state.sends.size() > 1) {
        auto* queued = state.sends.back(); state.sends.pop_back();
        Fail(*queued, ERROR_OPERATION_ABORTED);
    }
    GUID guid = WSAID_DISCONNECTEX; LPFN_DISCONNECTEX fn = nullptr; DWORD bytes = 0;
    if (WSAIoctl(socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &fn, sizeof(fn), &bytes, nullptr, nullptr) == SOCKET_ERROR)
        Fail(context, WSAGetLastError());
    else if (!fn(socket, &context.GetOver(), TF_REUSE_SOCKET, 0)) {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) Fail(context, error);
    }
    return true;
}
bool IocpService::Post(ULONG_PTR key, IoContext& context) {
    std::lock_guard lock(mutex_);
    if (stopping_ || context.pending.exchange(true)) return false;
    context.operation = IoOperation::AppEvent; context.socket = INVALID_SOCKET; context.key = key;
    context.GetOver() = {}; context.error = 0; ++pending_;
    if (!PostQueuedCompletionStatus(handle_, 1, key, &context.GetOver())) throw SocketError("post app event", GetLastError());
    return true;
}
void IocpService::Complete(IoContext& context) {
    if (context.socket != INVALID_SOCKET) {
        auto& state = State(context.socket);
        --state.pending;
        if (context.operation == IoOperation::Disconnect) state.disconnecting = false;
        if (state.disconnected && state.pending == 1) {
            auto* disconnected = state.disconnected; state.disconnected = nullptr;
            disconnected->deferred = true;
            PostQueuedCompletionStatus(handle_, 0, disconnected->key, &disconnected->GetOver());
        }
    }
    context.pending.store(false);
    if (--pending_ == 0 && stopping_) Wake();
}
bool IocpService::Poll(Completion& completion) {
    completion.Clear();
    for (;;) {
        DWORD bytes = 0; ULONG_PTR key = 0; OVERLAPPED* over = nullptr;
        BOOL ok = GetQueuedCompletionStatus(handle_, &bytes, &key, &over, INFINITE);
        DWORD error = ok ? 0 : GetLastError();
        if (!over) { if (stopping_ && pending_ == 0) return false; if (!ok) throw SocketError("GQCS", error); continue; }
        auto* context = IoContext::FromOver(over);
        std::shared_ptr<std::recursive_mutex> dispatch;
        if (context->socket != INVALID_SOCKET) {
            std::lock_guard lock(mutex_);
            dispatch = State(context->socket).dispatch;
        } else {
            std::lock_guard lock(mutex_);
            auto& slot = appDispatch_[key];
            if (!slot) slot = std::make_shared<std::recursive_mutex>();
            dispatch = slot;
        }
        // 코어 상태 잠금을 해제한 뒤 callback 실행권을 기다려 잠금 순환을 막는다.
        std::unique_lock<std::recursive_mutex> dispatchLock;
        if (dispatch) dispatchLock = std::unique_lock(*dispatch);
        std::lock_guard lock(mutex_);
        if (context->error) error = context->error;
        if (context->operation == IoOperation::Send) {
            auto& state = State(context->socket);
            if (!error && (!bytes || bytes > context->length - context->offset)) error = WSAECONNRESET;
            if (!error) {
                sent_ += bytes; context->offset += bytes;
                if (context->offset < context->length) {
                    if (!stopping_ && state.socket != INVALID_SOCKET) { IssueSend(state, *context); continue; }
                    error = ERROR_OPERATION_ABORTED;
                }
            }
            if (!state.sends.empty() && state.sends.front() == context) {
                state.sends.pop_front();
                if (!state.sends.empty() && !stopping_ && state.socket != INVALID_SOCKET && !state.disconnecting) IssueSend(state, *state.sends.front());
            }
        }
        if (context->operation == IoOperation::Disconnect && !context->deferred && State(context->socket).pending > 1) {
            context->error = error; State(context->socket).disconnected = context; continue;
        }
        if (context->operation == IoOperation::Receive && !error) received_ += bytes;
        if (context->operation == IoOperation::Accept && !error && !stopping_) {
            SOCKET listener = context->socket;
            if (setsockopt(context->listener, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char*>(&listener), sizeof(listener)) == SOCKET_ERROR)
                error = WSAGetLastError();
        }
        if (error) ++errors_;
        Complete(*context);
        completion = {context, key, bytes, error, std::move(dispatch), std::move(dispatchLock)};
        return true;
    }
}
void IocpService::Wake() { for (size_t i = 0; i < workers_; ++i) PostQueuedCompletionStatus(handle_, 0, 0, nullptr); }
void IocpService::Close(SOCKET socket) {
    std::lock_guard lock(mutex_);
    auto& state = State(socket);
    if (state.socket != INVALID_SOCKET) {
        CancelIoEx(reinterpret_cast<HANDLE>(socket), nullptr);
        closesocket(socket); state.socket = INVALID_SOCKET;
    }
    while (state.sends.size() > 1) {
        auto* context = state.sends.back(); state.sends.pop_back();
        Fail(*context, ERROR_OPERATION_ABORTED);
    }
}
void IocpService::RequestStop(size_t workers) {
    std::lock_guard lock(mutex_);
    workers_ = (std::max)(workers, size_t{1});
    if (stopping_.exchange(true)) { if (pending_ == 0) Wake(); return; }
    for (auto& [socket, state] : sockets_) Close(socket);
    if (pending_ == 0) Wake();
}
void IocpService::Drain() { while (pending_ != 0) { Completion completion; Poll(completion); } }
void IocpService::Finish() {
    std::lock_guard lock(mutex_);
    if (pending_ != 0) throw std::logic_error("finish with pending I/O");
    for (auto& [socket, state] : sockets_) if (state.socket != INVALID_SOCKET) closesocket(socket);
    sockets_.clear(); appDispatch_.clear();
}
NetworkStats IocpService::Stats() const {
    std::lock_guard lock(mutex_);
    uint64_t count = 0; for (const auto& [socket, state] : sockets_) if (state.socket != INVALID_SOCKET) ++count;
    return {pending_.load(), received_.load(), sent_.load(), errors_.load(), count};
}
bool FrameDecoder::Extract(char* buffer, size_t bytes, size_t& remain, std::vector<Frame>& frames) {
    if (remain > 255 || bytes > IoContext::Capacity - remain) { remain = 0; return false; }
    const size_t available = remain + bytes; size_t offset = 0;
    while (offset < available) {
        const auto length = static_cast<unsigned char>(buffer[offset]);
        if (length < 2) { remain = 0; frames.clear(); return false; }
        if (available - offset < 2 || length > available - offset) break;
        frames.emplace_back(buffer + offset, buffer + offset + length); offset += length;
    }
    remain = available - offset;
    if (remain) std::memmove(buffer, buffer + offset, remain);
    return true;
}
namespace {
std::wstring StopName(DWORD pid) { return L"Local\\Wod.Server.Stop." + std::to_wstring(pid); }
}
ProcessStopSignal::ProcessStopSignal() {
    event_ = CreateEventW(nullptr, TRUE, FALSE, StopName(GetCurrentProcessId()).c_str());
    if (!event_) throw SocketError("stop event", GetLastError());
}
ProcessStopSignal::~ProcessStopSignal() { if (event_) CloseHandle(event_); }
void ProcessStopSignal::Wait(std::chrono::milliseconds timeout) {
    const DWORD value = timeout == (std::chrono::milliseconds::max)() ? INFINITE : static_cast<DWORD>((std::min)(timeout.count(), int64_t{MAXDWORD - 1}));
    if (WaitForSingleObject(event_, value) == WAIT_FAILED) throw SocketError("wait stop", GetLastError());
}
bool ProcessStopSignal::Request(DWORD pid) {
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, StopName(pid).c_str());
    if (!event) return false;
    const bool ok = SetEvent(event) != FALSE; CloseHandle(event); return ok;
}
}
