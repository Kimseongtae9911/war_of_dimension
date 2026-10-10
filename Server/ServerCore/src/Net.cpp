#include <ServerCore/Net.h>
#include <ServerCore/Diagnostics.h>
#include <algorithm>
#include <cstring>
#include <limits>

namespace wod::core
{
namespace
{
std::unique_ptr<WinsockRuntime> runtimeWinsock;
std::unique_ptr<IocpService> runtimeService;
}

void NetworkRuntime::Start()
{
    if (runtimeService)
        return;

    runtimeWinsock = std::make_unique<WinsockRuntime>();
    try
    {
        runtimeService = std::make_unique<IocpService>();
    }
    catch (...)
    {
        runtimeWinsock.reset();
        throw;
    }
}

void NetworkRuntime::Stop()
{
    runtimeService.reset();
    runtimeWinsock.reset();
}

IocpService& NetworkRuntime::Get()
{
    if (!runtimeService)
        throw std::logic_error("network runtime not started");

    return *runtimeService;
}

SocketError::SocketError(const char *_operation, int _code) : std::runtime_error(std::string(_operation) + ": " + std::to_string(_code)), m_code(_code)
{
}

WinsockRuntime::WinsockRuntime()
{
    WSADATA data{};
    const int error = WSAStartup(MAKEWORD(2, 2), &data);
    if (error)
        throw SocketError("WSAStartup", error);
}

WinsockRuntime::~WinsockRuntime()
{
    WSACleanup();
}

SockAddr::SockAddr() : SockAddr(0)
{
}

SockAddr::SockAddr(unsigned short _port) : SockAddr(INADDR_ANY, _port)
{
}

SockAddr::SockAddr(unsigned int _addr, unsigned short _port)
{
    m_address.sin_family = AF_INET;
    m_address.sin_addr.s_addr = htonl(_addr);
    m_address.sin_port = htons(_port);
}

SockAddr::SockAddr(const sockaddr& _addr)
{
    std::memcpy(&m_address, &_addr, sizeof(m_address));
}

IoContext::IoContext() : m_native{{}, this}
{
    Reset();
}

IoContext *IoContext::FromOver(OVERLAPPED *_over)
{
    static_assert(offsetof(NativeRecord, m_over) == 0);

    return _over ? reinterpret_cast<NativeRecord *>(_over)->m_owner : nullptr;
}

void IoContext::ResetOver()
{
    if (m_pending.load())
        throw std::logic_error("reset pending OVERLAPPED");

    m_native.m_over = {};
}

void IoContext::Reset()
{
    ResetOver();
    m_buffer.buf = m_bytes;
    m_buffer.len = static_cast<ULONG>(m_Capacity);
    m_operation = IoOperation::Receive;
    m_socket = INVALID_SOCKET;
    m_key = 0;
    m_offset = m_length = 0;
    m_error = 0;
    m_deferred = false;
    m_listener = INVALID_SOCKET;
}

void IoContext::CopyPacket(std::span<const char> _packet)
{
    if (_packet.size() < 2 || _packet.size() > 255 || static_cast<unsigned char>(_packet[0]) != _packet.size())
        throw std::invalid_argument("invalid send frame size");

    Reset();
    m_length = _packet.size();
    m_buffer.len = static_cast<ULONG>(m_length);
    std::memcpy(m_bytes, _packet.data(), m_length);
}

IocpService::IocpService(SendCall _sendCall) : m_sendCall(std::move(_sendCall))
{
    m_handle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 0);
    if (!m_handle)
        throw SocketError("CreateIoCompletionPort", GetLastError());

    if (!m_sendCall)
        m_sendCall = [](SOCKET _socket, WSABUF *_buffer, OVERLAPPED *_over) {
            return WSASend(_socket, _buffer, 1, nullptr, 0, _over, nullptr);
        };
}

IocpService::~IocpService()
{
    RequestStop(1);
    Drain();
    Finish();
    if (m_handle)
        CloseHandle(m_handle);
}

SOCKET IocpService::CreateSocket()
{
    std::lock_guard lock(m_mutex);
    if (m_stopping)
        throw std::logic_error("socket after stop");

    SOCKET socket = WSASocketW(AF_INET, SOCK_STREAM, 0, nullptr, 0, WSA_FLAG_OVERLAPPED);
    if (socket == INVALID_SOCKET)
        throw SocketError("WSASocket", WSAGetLastError());

    if (auto old = m_sockets.find(socket); old != m_sockets.end())
    {
        if (old->second.m_pending)
        {
            closesocket(socket);
            throw std::logic_error("socket handle reused before drain");
        }

        m_sockets.erase(old);
    }

    m_sockets.try_emplace(socket);
    m_sockets.at(socket).m_socket = socket;

    return socket;
}

IocpService::SocketState& IocpService::State(SOCKET _socket)
{
    auto found = m_sockets.find(_socket);
    if (found == m_sockets.end())
        throw std::logic_error("unowned socket");

    return found->second;
}

void IocpService::Attach(SOCKET _socket, ULONG_PTR _key)
{
    std::lock_guard lock(m_mutex);
    auto& state = State(_socket);
    if (state.m_attached)
    {
        if (state.m_key != _key)
            throw std::logic_error("IOCP completion key is immutable");

        return;
    }

    if (!CreateIoCompletionPort(reinterpret_cast<HANDLE>(_socket), m_handle, _key, 0))
        throw SocketError("attach IOCP", GetLastError());

    state.m_key = _key;
    state.m_attached = true;
}

void IocpService::Connect(SOCKET _socket, const std::string& _ip, unsigned short _port)
{
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(_port);
    if (inet_pton(AF_INET, _ip.c_str(), &address.sin_addr) != 1)
        throw SocketError("IPv4 address", WSAEINVAL);

    if (WSAConnect(_socket, reinterpret_cast<sockaddr *>(&address), sizeof(address), nullptr, nullptr, nullptr, nullptr) == SOCKET_ERROR)
        throw SocketError("WSAConnect", WSAGetLastError());
}

void IocpService::Bind(SOCKET _socket, const SockAddr& _address)
{
    if (bind(_socket, _address.Native(), sizeof(sockaddr_in)) == SOCKET_ERROR)
        throw SocketError("bind", WSAGetLastError());
}

void IocpService::Listen(SOCKET _socket, int _backlog)
{
    if (listen(_socket, _backlog) == SOCKET_ERROR)
        throw SocketError("listen", WSAGetLastError());
}

bool IocpService::Begin(SOCKET _socket, IoContext& _context, IoOperation _op)
{
    auto& state = State(_socket);
    if (m_stopping || state.m_socket == INVALID_SOCKET || _context.m_pending.exchange(true))
        return false;

    _context.m_operation = _op;
    _context.m_socket = _socket;
    _context.m_key = state.m_key;
    _context.m_error = 0;
    _context.m_offset = 0;
    _context.m_deferred = false;
    _context.GetOver() = {};
    ++state.m_pending;
    ++m_pending;

    return true;
}

void IocpService::Fail(IoContext& _context, DWORD _error)
{
    _context.m_error = _error;
    if (!PostQueuedCompletionStatus(m_handle, 0, _context.m_key, &_context.GetOver()))
        throw SocketError("post failed I/O", GetLastError());
}

bool IocpService::Receive(SOCKET _socket, IoContext& _context)
{
    std::lock_guard lock(m_mutex);
    if (State(_socket).m_disconnecting || !Begin(_socket, _context, IoOperation::Receive))
        return false;

    DWORD flags = 0;
    if (WSARecv(_socket, &_context.GetWSA(), 1, nullptr, &flags, &_context.GetOver(), nullptr) == SOCKET_ERROR)
    {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING)
            Fail(_context, error);
    }

    return true;
}

void IocpService::IssueSend(SocketState& _state, IoContext& _context)
{
    _context.GetOver() = {};
    _context.GetWSA().buf = _context.GetSendBuf() + _context.m_offset;
    _context.GetWSA().len = static_cast<ULONG>(_context.m_length - _context.m_offset);
    if (m_sendCall(_state.m_socket, &_context.GetWSA(), &_context.GetOver()) == SOCKET_ERROR)
    {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING)
            Fail(_context, error);
    }
}

bool IocpService::Send(SOCKET _socket, IoContext& _context)
{
    std::lock_guard lock(m_mutex);
    auto& state = State(_socket);
    if (state.m_disconnecting || !Begin(_socket, _context, IoOperation::Send))
        return false;

    state.m_sends.push_back(&_context);
    if (state.m_sends.size() == 1)
        IssueSend(state, _context);

    return true;
}

bool IocpService::Accept(SOCKET _listener, SOCKET _socket, IoContext& _context)
{
    std::lock_guard lock(m_mutex);
    // AcceptEx 완료 key는 listener의 key다. 피연결 socket은 별도로 소유한다.
    if (!Begin(_listener, _context, IoOperation::Accept))
        return false;

    _context.m_listener = _socket;
    const DWORD size = sizeof(sockaddr_in) + 16;
    if (!AcceptEx(_listener, _socket, _context.GetSendBuf(), 0, size, size, nullptr, &_context.GetOver()))
    {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING)
            Fail(_context, error);
    }

    return true;
}

bool IocpService::Disconnect(SOCKET _socket, IoContext& _context)
{
    std::lock_guard lock(m_mutex);
    auto& state = State(_socket);
    if (state.m_disconnecting || !Begin(_socket, _context, IoOperation::Disconnect))
        return false;

    state.m_disconnecting = true;
    CancelIoEx(reinterpret_cast<HANDLE>(_socket), nullptr);
    while (state.m_sends.size() > 1)
    {
        auto *queued = state.m_sends.back();
        state.m_sends.pop_back();
        Fail(*queued, ERROR_OPERATION_ABORTED);
    }

    GUID guid = WSAID_DISCONNECTEX;
    LPFN_DISCONNECTEX fn = nullptr;
    DWORD bytes = 0;
    if (WSAIoctl(_socket, SIO_GET_EXTENSION_FUNCTION_POINTER, &guid, sizeof(guid), &fn, sizeof(fn), &bytes, nullptr, nullptr) == SOCKET_ERROR)
    {
        Fail(_context, WSAGetLastError());
    }
    else if (!fn(_socket, &_context.GetOver(), TF_REUSE_SOCKET, 0))
    {
        const int error = WSAGetLastError();
        if (error != WSA_IO_PENDING)
            Fail(_context, error);
    }

    return true;
}

bool IocpService::Post(ULONG_PTR _key, IoContext& _context)
{
    std::lock_guard lock(m_mutex);
    if (m_stopping || _context.m_pending.exchange(true))
        return false;

    _context.m_operation = IoOperation::AppEvent;
    _context.m_socket = INVALID_SOCKET;
    _context.m_key = _key;
    _context.GetOver() = {};
    _context.m_error = 0;
    ++m_pending;
    if (!PostQueuedCompletionStatus(m_handle, 1, _key, &_context.GetOver()))
        throw SocketError("post app event", GetLastError());

    return true;
}

void IocpService::Complete(IoContext& _context)
{
    if (_context.m_socket != INVALID_SOCKET)
    {
        auto& state = State(_context.m_socket);
        --state.m_pending;
        if (_context.m_operation == IoOperation::Disconnect)
            state.m_disconnecting = false;

        if (state.m_disconnected && state.m_pending == 1)
        {
            auto *disconnected = state.m_disconnected;
            state.m_disconnected = nullptr;
            disconnected->m_deferred = true;
            PostQueuedCompletionStatus(m_handle, 0, disconnected->m_key, &disconnected->GetOver());
        }
    }

    _context.m_pending.store(false);
    if (--m_pending == 0 && m_stopping)
        Wake();
}

bool IocpService::Poll(Completion& _completion)
{
    _completion.Clear();
    for (;;)
    {
        DWORD bytes = 0;
        ULONG_PTR key = 0;
        OVERLAPPED *over = nullptr;
        BOOL ok = GetQueuedCompletionStatus(m_handle, &bytes, &key, &over, INFINITE);
        DWORD error = ok ? 0 : GetLastError();
        if (!over)
        {
            if (m_stopping && m_pending == 0)
                return false;

            if (!ok)
                throw SocketError("GQCS", error);

            continue;
        }

        auto *context = IoContext::FromOver(over);
        std::shared_ptr<std::recursive_mutex> dispatch;
        if (context->m_socket != INVALID_SOCKET)
        {
            std::lock_guard lock(m_mutex);
            dispatch = State(context->m_socket).m_dispatch;
        }
        else
        {
            std::lock_guard lock(m_mutex);
            auto& slot = m_appDispatch[key];
            if (!slot)
                slot = std::make_shared<std::recursive_mutex>();

            dispatch = slot;
        }

        // 코어 상태 잠금을 해제한 뒤 callback 실행권을 기다려 잠금 순환을 막는다.
        std::unique_lock<std::recursive_mutex> dispatchLock;
        if (dispatch)
            dispatchLock = std::unique_lock(*dispatch);

        std::lock_guard lock(m_mutex);
        if (context->m_error)
            error = context->m_error;

        if (context->m_operation == IoOperation::Send)
        {
            auto& state = State(context->m_socket);
            if (!error && (!bytes || bytes > context->m_length - context->m_offset))
                error = WSAECONNRESET;

            if (!error)
            {
                m_sent += bytes;
                context->m_offset += bytes;
                if (context->m_offset < context->m_length)
                {
                    if (!m_stopping && state.m_socket != INVALID_SOCKET)
                    {
                        IssueSend(state, *context);
                        continue;
                    }

                    error = ERROR_OPERATION_ABORTED;
                }
            }

            if (!state.m_sends.empty() && state.m_sends.front() == context)
            {
                state.m_sends.pop_front();
                if (!state.m_sends.empty() && !m_stopping && state.m_socket != INVALID_SOCKET && !state.m_disconnecting)
                    IssueSend(state, *state.m_sends.front());
            }
        }

        if (context->m_operation == IoOperation::Disconnect && !context->m_deferred && State(context->m_socket).m_pending > 1)
        {
            context->m_error = error;
            State(context->m_socket).m_disconnected = context;
            continue;
        }

        if (context->m_operation == IoOperation::Receive && !error)
            m_received += bytes;

        if (context->m_operation == IoOperation::Accept && !error && !m_stopping)
        {
            SOCKET listener = context->m_socket;
            if (setsockopt(context->m_listener, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, reinterpret_cast<const char *>(&listener), sizeof(listener)) == SOCKET_ERROR)
                error = WSAGetLastError();
        }

        if (error)
            ++m_errors;

        Complete(*context);
        _completion = {context, key, bytes, error, std::move(dispatch), std::move(dispatchLock)};

        return true;
    }
}

void IocpService::Wake()
{
    for (size_t i = 0; i < m_workers; ++i)
        PostQueuedCompletionStatus(m_handle, 0, 0, nullptr);
}

void IocpService::Close(SOCKET _socket)
{
    std::lock_guard lock(m_mutex);
    auto& state = State(_socket);
    if (state.m_socket != INVALID_SOCKET)
    {
        CancelIoEx(reinterpret_cast<HANDLE>(_socket), nullptr);
        closesocket(_socket);
        state.m_socket = INVALID_SOCKET;
    }

    while (state.m_sends.size() > 1)
    {
        auto *context = state.m_sends.back();
        state.m_sends.pop_back();
        Fail(*context, ERROR_OPERATION_ABORTED);
    }
}

void IocpService::RequestStop(size_t _workers)
{
    std::lock_guard lock(m_mutex);
    m_workers = (std::max)(_workers, size_t{1});
    if (m_stopping.exchange(true))
    {
        if (m_pending == 0)
            Wake();

        return;
    }

    for (auto& [socket, state] : m_sockets)
        Close(socket);
    if (m_pending == 0)
        Wake();
}

void IocpService::Drain()
{
    while (m_pending != 0)
    {
        Completion completion;
        Poll(completion);
    }
}

void IocpService::Finish()
{
    std::lock_guard lock(m_mutex);
    if (m_pending != 0)
        throw std::logic_error("finish with pending I/O");

    for (auto& [socket, state] : m_sockets)
        if (state.m_socket != INVALID_SOCKET)
            closesocket(socket);

    m_sockets.clear();
    m_appDispatch.clear();
}

NetworkStats IocpService::Stats() const
{
    std::lock_guard lock(m_mutex);
    uint64_t count = 0;
    for (const auto& [socket, state] : m_sockets)
        if (state.m_socket != INVALID_SOCKET)
            ++count;

    return {m_pending.load(), m_received.load(), m_sent.load(), m_errors.load(), count};
}

bool FrameDecoder::Extract(char *_buffer, size_t _bytes, size_t& _remain, std::vector<Frame>& _frames)
{
    if (_remain > 255 || _bytes > IoContext::m_Capacity - _remain)
    {
        _remain = 0;

        return false;
    }

    const size_t available = _remain + _bytes;
    size_t offset = 0;
    while (offset < available)
    {
        const auto length = static_cast<unsigned char>(_buffer[offset]);
        if (length < 2)
        {
            _remain = 0;
            _frames.clear();

            return false;
        }

        if (available - offset < 2 || length > available - offset)
            break;

        _frames.emplace_back(_buffer + offset, _buffer + offset + length);
        offset += length;
    }

    _remain = available - offset;
    if (_remain)
        std::memmove(_buffer, _buffer + offset, _remain);

    return true;
}

namespace
{
std::wstring StopName(DWORD _pid)
{
    return L"Local\\Wod.Server.Stop." + std::to_wstring(_pid);
}
}

ProcessStopSignal::ProcessStopSignal()
{
    m_event = CreateEventW(nullptr, TRUE, FALSE, StopName(GetCurrentProcessId()).c_str());
    if (!m_event)
        throw SocketError("stop event", GetLastError());
}

ProcessStopSignal::~ProcessStopSignal()
{
    if (m_event)
        CloseHandle(m_event);
}

void ProcessStopSignal::Wait(std::chrono::milliseconds _timeout)
{
    const DWORD value = _timeout == (std::chrono::milliseconds::max)() ? INFINITE : static_cast<DWORD>((std::min)(_timeout.count(), int64_t{MAXDWORD - 1}));
    if (WaitForSingleObject(m_event, value) == WAIT_FAILED)
        throw SocketError("wait stop", GetLastError());
}

bool ProcessStopSignal::Request(DWORD _pid)
{
    HANDLE event = OpenEventW(EVENT_MODIFY_STATE, FALSE, StopName(_pid).c_str());
    if (!event)
        return false;

    const bool ok = SetEvent(event) != FALSE;
    CloseHandle(event);

    return ok;
}
}
