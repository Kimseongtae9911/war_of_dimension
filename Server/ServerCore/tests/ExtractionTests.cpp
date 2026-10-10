#include <ServerCore/SessionHandler.h>
#include <ServerCore/Concurrency.h>
#include <algorithm>
#include <cstring>
#include <future>

using namespace wod::core;

namespace
{
int checks = 0;

void Check(bool _condition, const char *_message)
{
    ++checks;
    if (!_condition)
        throw std::runtime_error(_message);
}

// 실제 서버 enum의 숫자 차이를 흉내 내어 named mapping을 확인한다.
enum class TestOperation
{
    OP_ACCEPT = 10,
    OP_RECV = 20,
    OP_SEND = 30,
    OP_DISCONNECT = 50
};
using Context = TaggedIoContext<TestOperation>;

class TestSession : public BasicSession<Context>
{
  public:
    TestSession(IocpService& _service, ObjectPool<Context>& _pool)
        : BasicSession(
              _service,
              [&_pool] {
                  return _pool.Acquire();
              },
              [&_pool](Context *_value) {
                  _pool.push(_value);
              })
    {
    }

    void Recv()
    {
        ++m_receives;
    }

    void Disconnect()
    {
        Invalidate();
        ++m_disconnects;
    }

    int m_receives = 0;
    int m_disconnects = 0;
};

class TestClient : public SessionHandler<TestSession>
{
  public:
    explicit TestClient(SessionRef _session) : m_session(std::move(_session))
    {
    }

    JobQueue m_jobs{JobBudget::Snapshot};
    std::vector<Frame> m_packets;
    std::vector<size_t> m_batches;
    bool m_allowDispatch = true;
    int m_disconnects = 0;

  private:
    SessionRef GetTransportSession() const override
    {
        return m_session;
    }

    bool ValidateFrame(std::span<const char> _frame) const override
    {
        return _frame[1] == 7;
    }

    bool DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation) override
    {
        if (!m_allowDispatch)
            return false;

        m_jobs.PushJob([this, session = _session, generation = _generation, frame = std::move(_frame)] {
            session->WithGeneration(generation, [&] {
                m_packets.push_back(frame);
            });
        });

        return true;
    }

    void OnReceiveComplete(size_t _frames) override
    {
        m_batches.push_back(_frames);
    }

    void OnDisconnectRequested() override
    {
        ++m_disconnects;
    }

    SessionRef m_session;
};

void ClientReceive()
{
    IocpService service;
    ObjectPool<Context> pool;
    auto session = std::make_shared<TestSession>(service, pool);
    TestClient client(session);
    auto& over = session->GetOverEx();
    auto *buffer = over.GetSendBuf();
    buffer[0] = 3;
    client.Receive(1, &over);
    Check(client.m_batches == std::vector<size_t>{0} && session->m_receives == 1, "partial header rearms without dispatch");
    std::memcpy(buffer + 1, "\x07x\x02\x07", 4);
    client.Receive(4, &over);
    std::memset(buffer, 0, IoContext::m_Capacity);
    client.m_jobs.ProcessJob();
    Check(client.m_packets.size() == 2 && client.m_batches.back() == 2, "coalesced frames dispatch through virtual hook");
    Check(std::find(client.m_packets.begin(), client.m_packets.end(), FrameDecoder::Frame{3, 7, 'x'}) != client.m_packets.end(), "queued frame owns bytes after receive buffer reuse");
    std::memcpy(buffer, "\x02\x07", 2);
    client.Receive(2, &over);
    session->Invalidate();
    client.m_jobs.ProcessJob();
    Check(client.m_packets.size() == 2, "queued dispatch retains generation guard");
    buffer[0] = 1;
    const auto receives = session->m_receives;
    client.Receive(1, &over);
    Check(client.m_disconnects == 1 && session->m_disconnects == 1 && session->m_receives == receives, "invalid length disconnects without rearm");
    std::memcpy(buffer, "\x02\x08", 2);
    client.Receive(2, &over);
    Check(client.m_disconnects == 2 && !client.m_jobs.HasJobs(), "endpoint validator rejects before dispatch");
    client.m_allowDispatch = false;
    std::memcpy(buffer, "\x02\x07", 2);
    client.Receive(2, &over);
    Check(client.m_disconnects == 3 && session->m_receives == receives, "content dispatch rejection disconnects");
}

void ContextAndDisconnect()
{
    Context context;
    for (auto [native, expected] : {std::pair{IoOperation::Accept, TestOperation::OP_ACCEPT}, {IoOperation::Receive, TestOperation::OP_RECV}, {IoOperation::Send, TestOperation::OP_SEND}, {IoOperation::Disconnect, TestOperation::OP_DISCONNECT}})
    {
        context.SetTransportOperation(native);
        Check(context.GetOP() == expected, "server operation mapped by name");
    }
    context.SetSocketID(42);
    context.SetInfo(3);
    context.SetSessionGeneration(7);
    context.Reset();
    Check(context.GetSocketID() == 42 && context.GetInfo() == 3 && !context.HasSessionGeneration(), "reset preserves accept cookie and clears generation presence");
    IocpService service;
    ObjectPool<Context> pool;
    BasicSession<Context> session(
        service,
        [&] {
            return pool.Acquire();
        },
        [&](Context *_value) {
            pool.push(_value);
        },
        true);
    service.Attach(session.GetSocket(), 42);
    const auto generation = session.Generation();
    service.RequestStop(1);
    session.Disconnect();
    Check(session.Generation() != generation && pool.Leased() == 0, "stopped transport returns disconnect context and invalidates generation");
    service.Drain();
    service.Finish();
    Check(service.Stats().m_pending == 0 && service.Stats().m_sockets == 0, "disconnect stop drains resources");
    pool.Clear();
}

class TestTarget : public JobTarget
{
  public:
    TestTarget() : JobTarget(JobBudget::Five)
    {
    }

    void OnJobQueueDisconnected() override
    {
        m_cleaned.set_value();
    }

    std::promise<void> m_cleaned;
};

void ScheduledJobs()
{
    JobScheduler scheduler;
    TestTarget target;
    std::atomic_int count = 0;
    std::promise<void> entered, release, all;
    auto proceed = release.get_future();
    auto enteredFuture = entered.get_future();
    auto allFuture = all.get_future();
    auto cleaned = target.m_cleaned.get_future();
    target.GetJobQueue()->PushJob([&] {
        entered.set_value();
        proceed.wait();
    });
    std::thread worker([&] {
        scheduler.ProcessJob();
    });
    scheduler.AddSessionQueue(&target);
    const bool started = enteredFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    const bool marked = target.IsInQueue();
    for (int i = 0; i < 12; ++i)
        target.GetJobQueue()->PushJob([&] {
            if (++count == 12)
                all.set_value();
        });
    scheduler.AddSessionQueue(&target);
    release.set_value();
    const bool completed = allFuture.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    target.SetDisconnected();
    scheduler.AddSessionQueue(&target);
    const bool reset = cleaned.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    scheduler.Stop();
    worker.join();
    Check(started && marked, "scheduler marks executing target");
    Check(completed && count == 12, "scheduler reschedules beyond five budget without duplicate execution");
    Check(reset && !target.IsInQueue() && !target.GetJobQueue()->HasJobs(), "disconnect cleanup runs after unmark and clear");
    target.ResetDisconnected();
    Check(!target.IsDisconnected(), "reused target clears disconnect state");
    scheduler.AddSessionQueue(&target);
    Check(!target.IsInQueue(), "stopped scheduler rejects new work");

    JobScheduler idle;
    std::thread waiting([&] {
        idle.ProcessJob();
    });
    idle.Stop();
    waiting.join();
    Check(true, "stop wakes idle scheduler");

    JobScheduler canceled;
    TestTarget disconnected;
    int staleCalls = 0;
    auto cleared = disconnected.m_cleaned.get_future();
    disconnected.GetJobQueue()->PushJob([&] {
        ++staleCalls;
    });
    disconnected.SetDisconnected();
    std::thread cleanup([&] {
        canceled.ProcessJob();
    });
    canceled.AddSessionQueue(&disconnected);
    const bool discarded = cleared.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    canceled.Stop();
    cleanup.join();
    Check(discarded && staleCalls == 0 && !disconnected.GetJobQueue()->HasJobs(), "disconnected target discards queued content");
}
}

int RunExtractionTests()
{
    ClientReceive();
    ContextAndDisconnect();
    ScheduledJobs();

    return checks;
}
