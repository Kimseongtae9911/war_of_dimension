#include <ServerCore/Concurrency.h>
#include <Windows.h>
#include <Psapi.h>
#include <algorithm>
#include <array>
#include <barrier>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <string>

#pragma comment(lib, "Psapi.lib")

using namespace wod::core;

namespace
{
using Clock = std::chrono::steady_clock;

uint64_t Nanoseconds()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now().time_since_epoch()).count());
}

uint64_t CpuTicks()
{
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        throw std::runtime_error("GetProcessTimes failed");

    const auto ticks = [](FILETIME _value) {
        return (uint64_t{_value.dwHighDateTime} << 32) | _value.dwLowDateTime;
    };

    return ticks(kernel) + ticks(user);
}

uint64_t PrivateBytes()
{
    PROCESS_MEMORY_COUNTERS_EX memory{};
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory), sizeof(memory)))
        throw std::runtime_error("GetProcessMemoryInfo failed");

    return memory.PrivateUsage;
}

void ConfigureMetrics(bool _enabled)
{
    JobMetrics::m_enabled = _enabled;
    JobMetrics::m_submitted = 0;
    JobMetrics::m_started = 0;
    JobMetrics::m_completed = 0;
    JobMetrics::m_failed = 0;
    JobMetrics::m_cleared = 0;
    JobMetrics::m_running = 0;
    JobMetrics::m_executeUs = 0;
    JobMetrics::m_maxExecuteUs = 0;
}

void Distribution(const char *_name, std::vector<uint64_t> _values)
{
    std::cout << ",\"" << _name << "\":";
    if (_values.empty())
    {
        std::cout << "null";

        return;
    }

    std::sort(_values.begin(), _values.end());
    const auto percentile = [&](double _fraction) {
        return _values[static_cast<size_t>(std::ceil(_fraction * static_cast<double>(_values.size()))) - 1] / 1000.0;
    };
    std::cout << "{\"samples\":" << _values.size() << ",\"p50_us\":" << percentile(.5) << ",\"p95_us\":" << percentile(.95) << ",\"p99_us\":" << percentile(.99) << ",\"max_us\":" << _values.back() / 1000.0 << '}';
}

struct alignas(64) QueueCounters
{
    uint64_t m_completed = 0;
    uint64_t m_checksum = 0;
    std::array<uint64_t, 6> m_padding{};
};

struct Workload
{
    std::vector<std::unique_ptr<JobQueue>> m_queues;
    std::vector<QueueCounters> m_counters;
    std::unique_ptr<std::atomic_uint8_t[]> m_seen;
    std::vector<uint64_t> m_enqueued, m_wait, m_finish;
    std::atomic_uint64_t m_duplicates = 0;
    bool m_detail = false, m_mixed = false;

    void Execute(size_t _id)
    {
        const bool sampled = m_detail && _id % 67 == 0;
        const auto begin = sampled ? Nanoseconds() : 0;
        if (m_detail && m_seen[_id].fetch_add(1, std::memory_order_relaxed) != 0)
            m_duplicates.fetch_add(1, std::memory_order_relaxed);

        if (m_mixed && _id % 997 == 0)
        {
            const auto until = Clock::now() + std::chrono::microseconds(50);
            while (Clock::now() < until)
            {
                std::atomic_signal_fence(std::memory_order_seq_cst);
            }
        }

        auto& counter = m_counters[_id % m_queues.size()];
        // 동일 큐의 callback은 실제 JobQueue의 실행 mutex로 직렬화된다.
        ++counter.m_completed;
        counter.m_checksum += static_cast<uint64_t>(_id) + 1;
        if (sampled)
        {
            m_wait[_id / 67] = begin - m_enqueued[_id / 67];
            m_finish[_id / 67] = Nanoseconds() - m_enqueued[_id / 67];
        }
    }
};

void QueueRun(const std::string& _mode, JobBudget _budget, size_t _jobs, bool _metrics, bool _detail, bool _emit = true)
{
    const size_t producers = (_mode == "producers4" || _mode == "consumers4" || _mode == "queues32" || _mode == "mixed") ? 4 : 1;
    const size_t consumers = (_mode == "consumers4" || _mode == "queues32" || _mode == "mixed") ? 4 : 1;
    const size_t queues = (_mode == "queues32" || _mode == "mixed") ? 32 : 1;
    const bool burst = _mode == "burst", preallocated = _mode == "preallocated";
    Workload work;
    work.m_detail = _detail;
    work.m_mixed = _mode == "mixed";
    work.m_counters.resize(queues);
    for (size_t index = 0; index < queues; ++index)
        work.m_queues.push_back(std::make_unique<JobQueue>(_budget));

    if (_detail)
    {
        work.m_seen = std::make_unique<std::atomic_uint8_t[]>(_jobs);
        work.m_enqueued.resize((_jobs + 66) / 67);
        work.m_wait.resize(work.m_enqueued.size());
        work.m_finish.resize(work.m_enqueued.size());
    }

    const auto callback = [&work](size_t _id) {
        return [&work, _id] {
            work.Execute(_id);
        };
    };
    std::vector<JobQueue::JobRef> retained;
    if (preallocated)
    {
        retained.reserve(_jobs);
        for (size_t id = 0; id < _jobs; ++id)
        {
            auto function = callback(id);
            retained.push_back(std::make_shared<Job<decltype(function)>>(std::move(function)));
        }
    }

    const auto enqueue = [&](size_t _id) {
        if (_detail && _id % 67 == 0)
            work.m_enqueued[_id / 67] = Nanoseconds();

        auto& queue = *work.m_queues[_id % queues];
        if (preallocated)
        {
            queue.PushJob(retained[_id]);
        }
        else
        {
            queue.PushJob(callback(_id));
        }
    };
    const auto preloadStart = Clock::now();
    if (burst)
        for (size_t id = 0; id < _jobs; ++id)
            enqueue(id);

    const double preloadMs = burst ? std::chrono::duration<double, std::milli>(Clock::now() - preloadStart).count() : 0;
    std::atomic_size_t producersDone = burst ? producers : 0;
    std::barrier start(static_cast<std::ptrdiff_t>((burst ? 0 : producers) + consumers + 1));
    std::vector<std::thread> threads;
    if (!burst)
    {
        for (size_t producer = 0; producer < producers; ++producer)
            threads.emplace_back([&, producer] {
                start.arrive_and_wait();
                for (size_t id = producer; id < _jobs; id += producers)
                    enqueue(id);

                producersDone.fetch_add(1);
            });
    }

    for (size_t consumer = 0; consumer < consumers; ++consumer)
        threads.emplace_back([&, consumer] {
            start.arrive_and_wait();
            for (;;)
            {
                // 여러 큐는 실행자별로 분할한다. 단일 큐는 같은 실행 mutex에서 경합한다.
                for (size_t queue = queues == 1 ? 0 : consumer; queue < queues; queue += queues == 1 ? 1 : consumers)
                    work.m_queues[queue]->ProcessJob();

                if (producersDone.load() == producers)
                {
                    bool pending = false;
                    for (size_t queue = queues == 1 ? 0 : consumer; queue < queues; queue += queues == 1 ? 1 : consumers)
                        pending = pending || work.m_queues[queue]->HasJobs();

                    if (!pending)
                        break;
                }
            }
        });

    const auto cpuBegin = CpuTicks(), privateBegin = PrivateBytes();
    const auto begin = Clock::now();
    start.arrive_and_wait();
    for (auto& thread : threads)
        thread.join();

    const double seconds = std::chrono::duration<double>(Clock::now() - begin).count();
    const auto cpuEnd = CpuTicks(), privateEnd = PrivateBytes();
    uint64_t completed = 0, checksum = 0;
    for (const auto& counter : work.m_counters)
    {
        completed += counter.m_completed;
        checksum += counter.m_checksum;
    }

    bool valid = completed == _jobs && checksum == uint64_t{_jobs} * (_jobs + 1) / 2 && work.m_duplicates == 0;
    for (const auto& queue : work.m_queues)
        valid = valid && !queue->HasJobs();

    if (_detail)
        for (size_t id = 0; id < _jobs; ++id)
            valid = valid && work.m_seen[id] == 1;

    if (_metrics)
        valid = valid && JobMetrics::m_completed == _jobs && JobMetrics::m_submitted == _jobs && JobMetrics::m_running == 0 && JobMetrics::m_failed == 0;

    if (!valid)
        throw std::runtime_error("job completion/uniqueness/metrics validation failed");

    if (!_emit)
        return;

    std::cout << "{\"ok\":true,\"case\":\"" << _mode << "\",\"budget\":\"" << (_budget == JobBudget::Five ? "Five" : "Snapshot") << "\",\"metrics\":" << (_metrics ? "true" : "false") << ",\"detail\":" << (_detail ? "true" : "false")
              << ",\"jobs\":" << completed << ",\"producers\":" << producers << ",\"consumers\":" << consumers << ",\"queues\":" << queues << ",\"seconds\":" << seconds << ",\"jobs_per_s\":" << completed / seconds
              << ",\"cpu_seconds\":" << (cpuEnd - cpuBegin) / 10000000.0 << ",\"private_begin\":" << privateBegin << ",\"private_end\":" << privateEnd << ",\"preload_ms\":" << preloadMs << ",\"uniqueness_checked\":" << (_detail ? "true" : "false");
    Distribution("enqueue_to_start", std::move(work.m_wait));
    Distribution("enqueue_to_finish", std::move(work.m_finish));
    std::cout << "}\n";
}

class BenchmarkTarget final : public JobTarget
{
  public:
    explicit BenchmarkTarget(JobBudget _budget) : JobTarget(_budget)
    {
    }

    void OnJobQueueDisconnected() override
    {
    }
};

void SchedulerRun(bool _idle, JobBudget _budget, size_t _jobs, bool _metrics)
{
    JobScheduler scheduler;
    BenchmarkTarget first(_budget), second(_budget);
    JobTarget *hot = std::less<JobTarget *>{}(&first, &second) ? &second : &first;
    JobTarget *cold = hot == &first ? &second : &first;
    uint64_t hotCompleted = 0, beforeCold = 0, coldWait = 0;
    bool coldRan = false;
    std::function<void()> repeat;
    repeat = [&] {
        ++hotCompleted;
        if (hotCompleted < _jobs)
        {
            hot->GetJobQueue()->PushJob(repeat);
        }
        else if (coldRan)
        {
            scheduler.Stop();
        }
    };
    uint64_t enqueued = 0;
    if (!_idle)
    {
        hot->GetJobQueue()->PushJob(repeat);
        cold->GetJobQueue()->PushJob([&] {
            coldWait = Nanoseconds() - enqueued;
            beforeCold = hotCompleted;
            coldRan = true;
            if (hotCompleted == _jobs)
                scheduler.Stop();
        });
        scheduler.AddSessionQueue(cold);
        scheduler.AddSessionQueue(hot);
    }

    std::barrier start(2);
    std::thread worker([&] {
        start.arrive_and_wait();
        scheduler.ProcessJob();
    });
    const auto cpuBegin = CpuTicks();
    const auto begin = Clock::now();
    enqueued = Nanoseconds();
    start.arrive_and_wait();
    if (_idle)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        scheduler.Stop();
    }

    worker.join();
    const double seconds = std::chrono::duration<double>(Clock::now() - begin).count();
    if (!_idle && (hotCompleted != _jobs || hot->GetJobQueue()->HasJobs() || cold->GetJobQueue()->HasJobs() || (_metrics && JobMetrics::m_completed != _jobs + 1)))
        throw std::runtime_error("scheduler completion validation failed");

    std::cout << "{\"ok\":true,\"case\":\"" << (_idle ? "idle" : "fairness") << "\",\"budget\":\"" << (_budget == JobBudget::Five ? "Five" : "Snapshot") << "\",\"metrics\":" << (_metrics ? "true" : "false")
              << ",\"detail\":false,\"jobs\":" << (_idle ? 0 : hotCompleted + 1) << ",\"seconds\":" << seconds << ",\"cpu_seconds\":" << (CpuTicks() - cpuBegin) / 10000000.0 << ",\"cold_wait_ms\":" << coldWait / 1000000.0
              << ",\"hot_completed_before_cold\":" << beforeCold << "}\n";
}
}

int RunJobQueueBenchmark(int _argc, char **_argv)
{
    try
    {
        if (_argc != 7)
            throw std::invalid_argument("--jobqueue-benchmark case Five|Snapshot metrics(0|1) detail(0|1) jobs(1..2000000)");

        const std::string mode = _argv[2], policy = _argv[3], metrics = _argv[4], detail = _argv[5];
        const std::vector<std::string> modes{"single", "producers4", "consumers4", "queues32", "burst", "preallocated", "mixed", "fairness", "idle"};
        size_t parsed = 0;
        const size_t jobs = std::stoull(_argv[6], &parsed);
        if (parsed != std::string(_argv[6]).size() || std::find(modes.begin(), modes.end(), mode) == modes.end() || (policy != "Five" && policy != "Snapshot") || (metrics != "0" && metrics != "1") || (detail != "0" && detail != "1") || jobs == 0 ||
            jobs > 2000000)
            throw std::invalid_argument("invalid benchmark arguments");

        ConfigureMetrics(metrics == "1");
        const auto budget = policy == "Five" ? JobBudget::Five : JobBudget::Snapshot;
        std::cout << std::setprecision(10);
        if (mode == "fairness" || mode == "idle")
        {
            SchedulerRun(mode == "idle", budget, jobs, metrics == "1");
        }
        else
        {
            QueueRun(mode, budget, std::min(jobs, size_t{20000}), metrics == "1", detail == "1", false);
            ConfigureMetrics(metrics == "1");
            QueueRun(mode, budget, jobs, metrics == "1", detail == "1");
        }

        ConfigureMetrics(false);

        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << '\n';

        return 1;
    }
}
