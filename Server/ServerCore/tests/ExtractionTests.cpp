#include <ServerCore/SessionHandler.h>
#include <ServerCore/Concurrency.h>
#include <ServerCore/TickProfile.h>
#include <algorithm>
#include <cstring>
#include <future>
#include "../../Game_Server/MatchTelemetry.h"
#include <regex>

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

class OrderedJob : public IJob
{
  public:
    std::function<void()> m_callback;

    void Execute() override
    {
        m_callback();
    }
};

void FairQueues()
{
    JobQueue queue(JobBudget::Snapshot);
    std::vector<JobQueue::JobRef> jobs;
    std::vector<int> order;
    for (int i = 0; i < 12; ++i)
        jobs.push_back(std::make_shared<OrderedJob>());

    std::sort(jobs.begin(), jobs.end());
    for (int i = 0; i < 12; ++i)
    {
        std::static_pointer_cast<OrderedJob>(jobs[i])->m_callback = [&, i] {
            order.push_back(i);
        };
        queue.PushJob(jobs[i]);
    }

    queue.ProcessJob();
    std::vector<int> expected;
    for (int i = 0; i < 12; ++i)
        expected.push_back(i);

    Check(order == expected, "FIFO preserves publication order independently of allocation address");

    JobQueue recursive(JobBudget::Five);
    auto first = std::make_shared<OrderedJob>();
    auto second = std::make_shared<OrderedJob>();
    if (first < second)
        std::swap(first, second);

    int repeats = 0;
    int atPacket = -1;
    first->m_callback = [&] {
        if (++repeats < 100)
            recursive.PushJob(first);
    };
    second->m_callback = [&] {
        atPacket = repeats;
    };
    recursive.PushJob(first);
    recursive.PushJob(second);
    recursive.ProcessJob();
    Check(atPacket == 1 && repeats == 4, "self-requeue cannot overtake an already queued packet");
    recursive.Clear();

    JobScheduler scheduler;
    TestTarget targets[2];
    auto *hot = &targets[0];
    auto *victim = &targets[1];
    if (std::less<TestTarget *>{}(hot, victim))
        std::swap(hot, victim);

    std::function<void()> repeat;
    int executed = 0;
    int atVictim = -1;
    repeat = [&] {
        if (++executed < 500)
            hot->GetJobQueue()->PushJob(repeat);
    };
    hot->GetJobQueue()->PushJob(repeat);
    victim->GetJobQueue()->PushJob([&] {
        atVictim = executed;
        scheduler.Stop();
    });
    scheduler.AddSessionQueue(hot);
    scheduler.AddSessionQueue(victim);
    scheduler.ProcessJob();
    Check(atVictim == 5, "ready targets rotate after one budget despite address and self-requeue");
    hot->GetJobQueue()->Clear();

    JobQueue exceptional(JobBudget::Snapshot);
    int survivors = 0;
    exceptional.PushJob([] {
        throw std::runtime_error("expected job failure");
    });
    exceptional.PushJob([&] {
        ++survivors;
    });
    bool threw = false;
    try
    {
        exceptional.ProcessJob();
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    exceptional.ProcessJob();
    Check(threw && survivors == 1 && !exceptional.HasJobs(), "callback failure retains remaining FIFO jobs and releases execution lock");

    JobQueue concurrent(JobBudget::Five);
    std::array<std::atomic_int, 4> previous = {-1, -1, -1, -1};
    std::atomic_int producersDone = 0;
    std::atomic_int calls = 0;
    std::atomic_int active = 0;
    std::atomic_bool ordered = true;
    std::vector<std::thread> workers;
    for (int producer = 0; producer < 4; ++producer)
        workers.emplace_back([&, producer] {
            for (int sequence = 0; sequence < 1000; ++sequence)
                concurrent.PushJob([&, producer, sequence] {
                    if (++active != 1 || previous[producer].exchange(sequence) != sequence - 1)
                        ordered = false;

                    ++calls;
                    --active;
                });

            ++producersDone;
        });

    for (int consumer = 0; consumer < 2; ++consumer)
        workers.emplace_back([&] {
            while (producersDone != 4 || concurrent.HasJobs())
            {
                concurrent.ProcessJob();
                std::this_thread::yield();
            }
        });

    for (auto& worker : workers)
        worker.join();

    Check(ordered && calls == 4000 && !concurrent.HasJobs(), "concurrent publication preserves each producer order with exactly-once serialized callbacks");
}

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

class RequeuedCleanupTarget : public JobTarget
{
  public:
    explicit RequeuedCleanupTarget(JobScheduler& _scheduler) : JobTarget(JobBudget::Five), m_scheduler(_scheduler)
    {
    }

    void OnJobQueueDisconnected() override
    {
        ++m_cleanups;
        m_scheduler.AddSessionQueue(this);
        if (m_cleanups == 1)
            m_requeued.set_value();
    }

    std::atomic_int m_cleanups = 0;
    std::promise<void> m_requeued;

  private:
    JobScheduler& m_scheduler;
};

void DisconnectCleanupOnce()
{
    JobScheduler scheduler;
    RequeuedCleanupTarget target(scheduler);
    auto requeued = target.m_requeued.get_future();
    std::thread worker([&] {
        scheduler.ProcessJob();
    });
    auto drain = [&] {
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
        while (target.IsInQueue() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::yield();
    };
    target.SetDisconnected();
    scheduler.AddSessionQueue(&target);
    const bool ready = requeued.wait_for(std::chrono::seconds(2)) == std::future_status::ready;
    drain();
    // callback에서 다시 등록된 대상까지 처리한 뒤 관찰한다.
    scheduler.Stop();
    worker.join();
    Check(ready && target.m_cleanups == 1 && !target.IsInQueue(), "requeued disconnect cleans up exactly once");
    target.ResetDisconnected();
    Check(target.TryBeginDisconnectCleanup(), "reuse rearms disconnect cleanup latch");
    Check(!target.TryBeginDisconnectCleanup(), "duplicate cleanup claim rejected");
}
}

void MatchObservationCopies()
{
    using Observation = wod_server::MatchTelemetry;
    Observation::Register(3, {"test0000", "test0001", "test0002", std::string("a\"\n")});
    Observation::Sample sample;
    sample.m_timestampMs = 1000;
    sample.m_gameMs = 42;
    sample.m_nextWaveMs = 43;
    sample.m_spawns.emplace_back(2, 5000);
    Observation::Publish(3, sample);
    sample.m_spawns.clear();
    const auto json = Observation::Json();
    Check(json.find("7465737430303030") != std::string::npos && json.find("61220a") != std::string::npos, "member byte keys include quotes/control bytes safely");
    Check(json.find("\"id\":2,\"due_ms\":5000") != std::string::npos, "published observation owns a detached value copy");
    std::thread writer([] {
        for (int value = 0; value < 1000; ++value)
        {
            Observation::Sample update;
            update.m_gameMs = value;
            update.m_nextWaveMs = value + 1;
            Observation::Publish(3, std::move(update));
        }
    });
    bool consistent = true;
    const std::regex values("\"game_ms\":([0-9]+),\"next_wave_ms\":([0-9]+)");
    for (int index = 0; index < 100; ++index)
    {
        const auto snapshot = Observation::Json();
        std::smatch match;
        consistent = consistent && std::regex_search(snapshot, match, values) && std::stoll(match[2]) == std::stoll(match[1]) + 1;
    }

    writer.join();
    Check(consistent, "reporter never reads mixed fields during concurrent publication");
    Observation::Register(3, {"new00000", "new00001", "new00002", "new00003"});
    const auto reset = Observation::Json();
    Check(reset.find("7465737430303030") == std::string::npos && reset.find("\"spawns\":[]") != std::string::npos, "new match registration clears old members and timers");
}

void TickProfileIsolation()
{
    using Profile = TickProfile;
    const auto calls = [](Profile::Stage _stage) {
        const std::string pattern = std::string("\"") + Profile::m_names[static_cast<size_t>(_stage)] + "\":\\{\"calls\":([0-9]+)";
        std::smatch result;
        const auto json = Profile::Json();
        if (!std::regex_search(json, result, std::regex(pattern)))
            throw std::runtime_error("missing profile stage");

        return std::stoull(result[1]);
    };
    Profile::Enable(false);
    const auto before = calls(Profile::Stage::Total);
    {
        Profile::Frame frame;
        Profile::Scope scope(Profile::Stage::Jobs);
    }
    Check(calls(Profile::Stage::Total) == before, "disabled profiling records no frame");
    Profile::Enable(true);
    {
        Profile::Scope outside(Profile::Stage::Jobs);
    }
    Check(calls(Profile::Stage::Jobs) == 0, "profiling ignores work outside tick context");
    try
    {
        Profile::Frame frame;
        Profile::Scope player(Profile::Stage::PlayerUpdate);
        Profile::Scope ray(Profile::Stage::HeightRay);
        ray.Work(17);
        throw std::runtime_error("unwind profile");
    }
    catch (const std::runtime_error&)
    {
    }
    {
        Profile::Scope outside(Profile::Stage::Jobs);
    }
    Check(calls(Profile::Stage::Total) == before + 1 && calls(Profile::Stage::Jobs) == 0, "exception restores profiling context");
    Check(calls(Profile::Stage::PlayerHeight) == 1 && calls(Profile::Stage::MinionHeight) == 0, "height ray attributed to player owner");
    Check(Profile::Json().find("\"work\":17") != std::string::npos, "ray visited work retained on unwind");
    const auto worker = [] {
        for (int index = 0; index < 100; ++index)
        {
            Profile::Frame frame;
            Profile::Scope minion(Profile::Stage::MinionUpdate);
            Profile::Scope ray(Profile::Stage::HeightRay);
        }
    };
    std::thread first(worker), second(worker);
    first.join();
    second.join();
    Check(calls(Profile::Stage::Total) == before + 201 && calls(Profile::Stage::MinionHeight) == 200 && calls(Profile::Stage::PlayerHeight) == 1, "parallel tick frames retain independent owners");
    Profile::Enable(false);
}

int RunExtractionTests()
{
    FairQueues();
    MatchObservationCopies();
    TickProfileIsolation();
    ClientReceive();
    ContextAndDisconnect();
    ScheduledJobs();
    DisconnectCleanupOnce();

    return checks;
}
