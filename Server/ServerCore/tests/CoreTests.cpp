#include <ServerCore/Concurrency.h>
#include <ServerCore/Diagnostics.h>
#include <ServerCore/Session.h>
#include <algorithm>
#include <array>
#include <iostream>
#include <thread>
#include <cstring>
#include <future>
#include <Protocol/Validation.h>

void DumpAbi();
using namespace wod::core;

namespace
{
int checks = 0;

void Check(bool _value, const char *_name)
{
    ++checks;
    if (!_value)
        throw std::runtime_error(_name);
}

template <class F> void ExpectThrow(F _f, const char *_name)
{
    bool threw = false;
    try
    {
        _f();
    }
    catch (...)
    {
        threw = true;
    }
    Check(threw, _name);
}

void Frames()
{
    char buffer[256]{};
    size_t remain = 0;
    std::vector<FrameDecoder::Frame> frames;
    buffer[0] = 4;
    Check(FrameDecoder::Extract(buffer, 1, remain, frames) && remain == 1 && frames.empty(), "one byte header");
    buffer[remain] = 8;
    Check(FrameDecoder::Extract(buffer, 1, remain, frames) && remain == 2 && frames.empty(), "split body");
    std::memcpy(buffer + remain, "ab\x03\x09x\x04", 6);
    Check(FrameDecoder::Extract(buffer, 6, remain, frames) && frames.size() == 2 && remain == 1 && buffer[0] == 4, "frames plus tail");
    Check(frames[0] == FrameDecoder::Frame({4, 8, 'a', 'b'}), "owned frame");
    for (const char invalid : {char{0}, char{1}})
    {
        remain = 0;
        frames.clear();
        buffer[0] = invalid;
        buffer[1] = 8;
        Check(!FrameDecoder::Extract(buffer, 2, remain, frames), "invalid length");
    }
    remain = 0;
    frames.clear();
    buffer[0] = static_cast<char>(255);
    buffer[1] = 7;
    Check(FrameDecoder::Extract(buffer, 255, remain, frames) && frames.size() == 1 && remain == 0, "maximum frame");
    remain = 255;
    Check(!FrameDecoder::Extract(buffer, 2, remain, frames), "receive overflow");
    for (int i = 0; i < 1000; ++i)
    {
        remain = 0;
        frames.clear();
        buffer[0] = 2;
        buffer[1] = 4;
        if (!FrameDecoder::Extract(buffer, 2, remain, frames) || frames.size() != 1)
            throw std::runtime_error("long stream");
    }
    ++checks;
}

void Jobs()
{
    JobQueue queue(JobBudget::Five);
    int count = 0;
    for (int i = 0; i < 12; ++i)
        queue.PushJob([&] {
            ++count;
        });
    queue.ProcessJob();
    Check(count == 5 && queue.HasJobs(), "lobby five budget");
    queue.ProcessJob();
    queue.ProcessJob();
    Check(count == 12 && !queue.HasJobs(), "remaining jobs");
    auto fn = [&] {
        ++count;
    };
    queue.PushJob(std::make_shared<Job<decltype(fn)>>(fn));
    queue.PushJob([owned = std::make_unique<int>(10), &count] {
        count += *owned;
    });
    queue.ProcessJob();
    Check(count == 23, "JobRef and move-only callable");
    JobQueue snapshot;
    int executed = 0;
    snapshot.PushJob([&] {
        ++executed;
        snapshot.PushJob([&] {
            ++executed;
        });
    });
    snapshot.ProcessJob();
    Check(executed == 1 && snapshot.HasJobs(), "snapshot budget");
    snapshot.ProcessJob();
    Check(executed == 2, "snapshot remainder");
    std::atomic_int concurrent = 0;
    std::vector<std::thread> producers;
    for (int p = 0; p < 4; ++p)
        producers.emplace_back([&] {
            for (int i = 0; i < 1000; ++i)
                snapshot.PushJob([&] {
                    ++concurrent;
                });
        });
    for (auto &thread : producers)
        thread.join();
    std::thread first([&] {
        snapshot.ProcessJob();
    });
    std::thread second([&] {
        snapshot.ProcessJob();
    });
    first.join();
    second.join();
    while (snapshot.HasJobs())
        snapshot.ProcessJob();

    Check(concurrent == 4000, "concurrent jobs exactly once");
}

void Pool()
{
    ObjectPool<IoContext> pool;
    auto *first = pool.Acquire();
    auto *second = pool.Acquire();
    Check(pool.Size() == 2 && pool.Leased() == 2, "pool growth");
    ExpectThrow(
        [&] {
            pool.Clear();
        },
        "cannot clear borrowed pool");
    first->m_pending = true;
    ExpectThrow(
        [&] {
            pool.push(first);
        },
        "pool rejects pending return");
    Check(pool.Leased() == 2, "failed return retains lease");
    first->m_pending = false;
    pool.push(first);
    ExpectThrow(
        [&] {
            pool.push(first);
        },
        "double return");
    auto *same = pool.Acquire();
    Check(same == first, "pool reuse");
    pool.push(second);
    pool.push(same);
    pool.Clear();
    Check(pool.Size() == 0 && pool.Leased() == 0, "pool teardown");
    IoContext context;
    context.m_pending = true;
    ExpectThrow(
        [&] {
            context.Reset();
        },
        "pending context reset");
    context.m_pending = false;
    const char invalid[] = {1};
    ExpectThrow(
        [&] {
            context.CopyPacket(invalid);
        },
        "send length check");
    Check(IoContext::FromOver(&context.GetOver()) == &context, "native owner recovery");
}

void SendFailureAndPartial()
{
    IocpService *owner = nullptr;
    int calls = 0;
    std::vector<char> submitted;
    IocpService service([&](SOCKET, WSABUF *_data, OVERLAPPED *_over) {
        const ULONG completed = (std::min)(_data->len, ULONG{1});
        submitted.insert(submitted.end(), _data->buf, _data->buf + completed);
        ++calls;
        PostQueuedCompletionStatus(owner->Handle(), completed, 7, _over);
        WSASetLastError(WSA_IO_PENDING);

        return SOCKET_ERROR;
    });
    owner = &service;
    auto socket = service.CreateSocket();
    service.Attach(socket, 7);
    IoContext first, second;
    const char a[] = {4, 3, 'a', 'b'}, b[] = {3, 4, 'c'};
    first.CopyPacket(a);
    second.CopyPacket(b);
    Check(service.Send(socket, first) && service.Send(socket, second), "queued send");
    Completion complete;
    Check(service.Poll(complete) && complete.m_context == &first && !complete.m_error && first.m_offset == 4, "partial send continuation");
    Check(service.Poll(complete) && complete.m_context == &second && !complete.m_error, "send order");
    Check(calls == 7 && submitted == std::vector<char>({4, 3, 'a', 'b', 3, 4, 'c'}), "no duplicate send bytes");
    service.RequestStop(1);
    service.Drain();
    service.Finish();
    Check(service.Stats().m_pending == 0 && service.Stats().m_sockets == 0, "partial stop cleanup");
    IocpService failed([](SOCKET, WSABUF *, OVERLAPPED *) {
        WSASetLastError(WSAECONNRESET);

        return SOCKET_ERROR;
    });
    auto failedSocket = failed.CreateSocket();
    failed.Attach(failedSocket, 8);
    IoContext error;
    error.CopyPacket(a);
    Check(failed.Send(failedSocket, error) && failed.Poll(complete) && complete.m_error == WSAECONNRESET && !error.m_pending, "immediate send error");
    failed.RequestStop(1);
    failed.Drain();
    failed.Finish();
}

void Loopback()
{
    IocpService service;
    const SOCKET listener = service.CreateSocket();
    service.Attach(listener, 1);
    service.Bind(listener, SockAddr(0));
    service.Listen(listener, 4);
    sockaddr_in address{};
    int length = sizeof(address);
    if (getsockname(listener, reinterpret_cast<sockaddr *>(&address), &length))
        throw SocketError("getsockname", WSAGetLastError());

    const SOCKET accepted = service.CreateSocket();
    service.Attach(accepted, 2);
    IoContext accept;
    Check(service.Accept(listener, accepted, accept), "accept submitted");
    const SOCKET client = service.CreateSocket();
    service.Connect(client, "127.0.0.1", ntohs(address.sin_port));
    service.Attach(client, 3);
    Completion completion;
    Check(service.Poll(completion) && completion.m_context == &accept && !completion.m_error, "real AcceptEx");
    IoContext recv, send;
    const char frame[] = {4, 7, 'o', 'k'};
    send.CopyPacket(frame);
    Check(service.Receive(accepted, recv) && service.Send(client, send), "real recv/send submitted");
    size_t received = 0;
    int finished = 0;
    while (finished < 2)
    {
        service.Poll(completion);
        Check(!completion.m_error, "loopback completion");
        if (completion.m_context == &recv)
            received = completion.m_bytes;

        ++finished;
    }

    Check(received == 4 && std::memcmp(recv.GetSendBuf(), frame, 4) == 0, "loopback bytes");
    recv.Reset();
    Check(service.Receive(accepted, recv), "pending receive before stop");
    service.RequestStop(1);
    service.Drain();
    service.Finish();
    Check(service.Stats().m_pending == 0 && service.Stats().m_sockets == 0 && !recv.m_pending, "cancel drain");
}

void ThreadLifetimes()
{
    std::atomic_bool stopping = false;
    std::atomic_int failures = 0, exited = 0;
    {
        ThreadGroup workers(
            [&] {
                stopping = true;
            },
            [&](std::exception_ptr) {
                ++failures;
            });
        workers.Launch([] {
            throw std::runtime_error("injected worker failure");
        });
        workers.Launch([&] {
            while (!stopping)
                std::this_thread::yield();

            ++exited;
        });
        workers.StopAndJoin();
        workers.StopAndJoin();
        Check(workers.Failed() && failures == 1 && exited == 1, "worker failure and idempotent join");
    }
    stopping = false;
    exited = 0;
    try
    {
        ThreadGroup workers(
            [&] {
                stopping = true;
            },
            [](std::exception_ptr) {
            });
        workers.Launch([&] {
            while (!stopping)
                std::this_thread::yield();

            ++exited;
        });
        throw std::runtime_error("partial startup");
    }
    catch (const std::runtime_error &)
    {
    }
    Check(exited == 1, "partial thread startup scope cleanup");
}

void CallbackSerialization()
{
    IocpService *owner = nullptr;
    IocpService service([&](SOCKET, WSABUF *_data, OVERLAPPED *_over) {
        PostQueuedCompletionStatus(owner->Handle(), _data->len, 5, _over);
        WSASetLastError(WSA_IO_PENDING);

        return SOCKET_ERROR;
    });
    owner = &service;
    const auto socket = service.CreateSocket();
    service.Attach(socket, 5);
    service.Attach(socket, 5);
    ExpectThrow(
        [&] {
            service.Attach(socket, 6);
        },
        "immutable completion key");
    IoContext first, second;
    const char data[] = {2, 4};
    first.CopyPacket(data);
    second.CopyPacket(data);
    service.Send(socket, first);
    service.Send(socket, second);
    std::promise<void> entered, release;
    auto proceed = release.get_future();
    std::atomic_bool secondFinished = false;
    std::thread worker([&] {
        Completion completion;
        if (!service.Poll(completion) || completion.m_context != &first)
            std::terminate();

        entered.set_value();
        proceed.wait();
        completion.Clear();
    });
    entered.get_future().wait();
    std::thread next([&] {
        Completion completion;
        service.Poll(completion);
        secondFinished = true;
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    const bool held = !secondFinished.load();
    release.set_value();
    worker.join();
    next.join();
    Check(held && secondFinished, "connection callbacks serialized across workers");
    service.RequestStop(2);
    service.Drain();
    service.Finish();
    Check(service.Stats().m_pending == 0, "multi-worker callback teardown");
}

void CanceledQueue()
{
    IocpService *owner = nullptr;
    IocpService service([&](SOCKET, WSABUF *, OVERLAPPED *_over) {
        PostQueuedCompletionStatus(owner->Handle(), 1, 6, _over);
        WSASetLastError(WSA_IO_PENDING);

        return SOCKET_ERROR;
    });
    owner = &service;
    auto socket = service.CreateSocket();
    service.Attach(socket, 6);
    IoContext first, second;
    const char data[] = {4, 4, 'a', 'b'};
    first.CopyPacket(data);
    second.CopyPacket(data);
    service.Send(socket, first);
    service.Send(socket, second);
    service.RequestStop(1);
    Completion completion;
    int canceled = 0;
    while (service.Poll(completion))
    {
        if (completion.m_error == ERROR_OPERATION_ABORTED)
            ++canceled;
    }

    Check(canceled == 2 && !first.m_pending && !second.m_pending, "active partial and queued send cancellation");
    service.Finish();
    Check(!service.Stats().m_pending && !service.Stats().m_sockets, "queued stop drain");
}

struct SessionContext : IoContext
{
    void SetTransportOperation(IoOperation _op)
    {
        m_operation = _op;
    }
};

void GenerationAndValidation()
{
    IocpService service;
    BasicSession<SessionContext> session(
        service,
        [] {
            return new SessionContext;
        },
        [](auto *_p) {
            delete _p;
        });
    int calls = 0;
    const auto old = session.Generation();
    session.WithGeneration(old, [&] {
        ++calls;
    });
    session.Invalidate();
    session.WithGeneration(old, [&] {
        ++calls;
    });
    Check(calls == 1, "stale generation skipped");
    session.WithGeneration(session.Generation(), [&] {
        ++calls;
    });
    Check(calls == 2, "current generation executes");
    CS_READY_PACKET ready{};
    ready.size = sizeof(ready);
    ready.type = CS_READY;
    ready.id = 3;
    ready.ready = true;
    const auto bytes = std::span(reinterpret_cast<const char *>(&ready), sizeof(ready));
    Check(wod::protocol::Validate(bytes, wod::protocol::Endpoint::GameClient), "game endpoint frame");
    Check(!wod::protocol::Validate(bytes, wod::protocol::Endpoint::LobbyClient), "wrong endpoint rejected");
    Check(!wod::protocol::Validate(bytes.first(2), wod::protocol::Endpoint::GameClient), "struct size rejected");
    const unsigned char expected[] = {7, CS_READY, 3, 0, 0, 0, 1};
    Check(sizeof(ready) == sizeof(expected) && std::memcmp(&ready, expected, sizeof(expected)) == 0, "packed representative packet bytes");
    const char unknown[] = {2, static_cast<char>(255)};
    Check(!wod::protocol::Validate(unknown, wod::protocol::Endpoint::GameClient), "unknown type rejected");
    CS_SKILL_SELECT_PACKET skill{};
    skill.size = sizeof(skill);
    skill.type = CS_SKILL_SELECT;
    skill.id = 0;
    skill.storage = MAX_SKILL;
    Check(!wod::protocol::Validate(std::span(reinterpret_cast<const char *>(&skill), sizeof(skill)), wod::protocol::Endpoint::GameClient), "skill storage bounds");
    CS_LOGIN_PACKET login{};
    login.size = sizeof(login);
    login.type = CS_LOGIN;
    std::memset(login.name, 'x', sizeof(login.name));
    Check(!wod::protocol::Validate(std::span(reinterpret_cast<const char *>(&login), sizeof(login)), wod::protocol::Endpoint::LobbyClient), "unterminated name rejected");
    LG_MATCH_START_PACKET start{};
    start.size = sizeof(start);
    start.type = LG_MATCH_START;
    start.match_num = -1;
    Check(!wod::protocol::Validate(std::span(reinterpret_cast<const char *>(&start), sizeof(start)), wod::protocol::Endpoint::LobbyToGame), "server match bounds");
    LG_MATCH_PACKET player{};
    player.size = sizeof(player);
    player.type = LG_MATCH_PLAYER;
    player.id = 4;
    Check(!wod::protocol::Validate(std::span(reinterpret_cast<const char *>(&player), sizeof(player)), wod::protocol::Endpoint::LobbyToGame), "server player bounds");
    const auto socket = service.CreateSocket();
    ExpectThrow(
        [&] {
            service.Connect(socket, "invalid", 1);
        },
        "connect failure cleanup");
    service.RequestStop(1);
    service.Drain();
    service.Finish();
    Check(!service.Stats().m_sockets, "partial initialization socket cleanup");
}

void MultiWorkerDisconnect()
{
    IocpService service;
    const auto listener = service.CreateSocket();
    service.Attach(listener, 1);
    service.Bind(listener, SockAddr(0));
    service.Listen(listener, 4);
    sockaddr_in address{};
    int length = sizeof(address);
    getsockname(listener, reinterpret_cast<sockaddr *>(&address), &length);
    const auto accepted = service.CreateSocket();
    service.Attach(accepted, 2);
    IoContext accept;
    service.Accept(listener, accepted, accept);
    const auto client = service.CreateSocket();
    service.Connect(client, "127.0.0.1", ntohs(address.sin_port));
    service.Attach(client, 3);
    Completion completion;
    service.Poll(completion);
    completion.Clear();
    IoContext receive, disconnect;
    service.Receive(accepted, receive);
    service.Close(client);
    service.Disconnect(accepted, disconnect);
    std::atomic_int finished = 0;
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i)
        workers.emplace_back([&] {
            Completion event;
            while (service.Poll(event))
                ++finished;
        });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (finished < 2 && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();

    const bool drained = finished == 2;
    service.RequestStop(workers.size());
    for (auto &worker : workers)
        worker.join();
    service.Finish();
    Check(drained && !receive.m_pending && !disconnect.m_pending, "DisconnectEx drains receive across workers");
    Check(!service.Stats().m_pending && !service.Stats().m_sockets, "multi-worker normal stop");
}

}

int main(int argc, char **argv)
{
    ConfigureProcessDiagnostics();
    if (argc > 1 && std::string(argv[1]) == "--abi")
    {
        DumpAbi();

        return 0;
    }

    if (argc > 2 && std::string(argv[1]) == "--stop")
        return ProcessStopSignal::Request(static_cast<DWORD>(std::stoul(argv[2]))) ? 0 : 1;

    try
    {
        WinsockRuntime runtime;
        Frames();
        Jobs();
        Pool();
        SendFailureAndPartial();
        ThreadLifetimes();
        CallbackSerialization();
        CanceledQueue();
        GenerationAndValidation();
        MultiWorkerDisconnect();
        for (int i = 0; i < 5; ++i)
            Loopback();
        std::cout << "{\"ok\":true,\"checks\":" << checks << "}\n";

        return 0;
    }
    catch (const std::exception &ex)
    {
        std::cerr << ex.what() << '\n';

        return 1;
    }
}
