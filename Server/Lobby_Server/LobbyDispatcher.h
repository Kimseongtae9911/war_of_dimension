#pragma once
#include <ServerCore/Concurrency.h>
#include <deque>
#include <shared_mutex>
#include <sstream>

namespace wod_server
{
// 채널별 실행은 직렬이며, 서로 다른 채널은 공유 worker pool에서 병렬 실행한다.
// 여러 채널의 가변 상태에 접근하는 기존 기능은 전역 배타 작업으로 격리한다.
class LobbyDispatcher
{
  public:
    struct Work
    {
        bool m_global;
        std::function<void()> m_execute;
        uint64_t m_enqueuedUs;
    };

    // 세션 작업의 순서를 유지하고 현재 소유 채널 또는 전역 큐로 전달한다.
    class SessionWorkQueue
    {
      public:
        void SetChannel(int _channel)
        {
            std::lock_guard lock(m_mutex);
            m_channel = _channel;
        }

        void Clear()
        {
            std::lock_guard lock(m_mutex);
            m_work.clear();
        }

      private:
        friend class LobbyDispatcher;
        std::mutex m_mutex;
        std::deque<Work> m_work;
        int m_channel = -1;
        bool m_scheduled = false;
        bool m_paused = false;
    };

    explicit LobbyDispatcher(size_t _channels)
    {
        for (size_t i = 0; i < _channels; ++i)
            m_channels.push_back(std::make_unique<Target>());
    }

    bool Post(SessionWorkQueue& _workQueue, bool _global, std::function<void()> _work)
    {
        std::lock_guard lock(_workQueue.m_mutex);
        if (m_stopping.load())
            return false;

        _workQueue.m_work.push_back({_global, std::move(_work), Timestamp()});
        ++m_pending;
        if (!_workQueue.m_scheduled && !_workQueue.m_paused)
            ScheduleLocked(_workQueue);

        return true;
    }

    // 현재 세션 작업 안에서 호출한다. SQL 대기 동안 worker와 world lock은 반환한다.
    void Suspend(SessionWorkQueue& _workQueue)
    {
        std::lock_guard lock(_workQueue.m_mutex);
        _workQueue.m_paused = true;
    }

    bool Resume(SessionWorkQueue& _workQueue, std::function<void()> _completion)
    {
        std::lock_guard lock(_workQueue.m_mutex);
        if (m_stopping.load())
            return false;

        _workQueue.m_work.push_front({true, std::move(_completion), Timestamp()});
        ++m_pending;
        _workQueue.m_paused = false;
        if (!_workQueue.m_scheduled)
            ScheduleLocked(_workQueue);

        return true;
    }

    bool PostGlobal(std::function<void()> _work)
    {
        if (m_stopping.load())
            return false;

        ++m_global.m_pending;
        m_global.GetJobQueue()->PushJob([this, work = std::move(_work), queued = Timestamp()] {
            std::unique_lock gate(m_world);
            --m_global.m_pending;
            const auto start = Timestamp();
            work();
            if (queued)
            {
                m_global.m_waitUs += start - queued;
                m_global.m_busyUs += Timestamp() - start;
            }

            ++m_globalJobs;
        });
        m_scheduler.AddSessionQueue(&m_global);

        return true;
    }

    void ProcessJob()
    {
        m_scheduler.ProcessJob();
    }

    std::unique_lock<std::shared_mutex> LockGlobal()
    {
        return std::unique_lock(m_world);
    }

    void Stop()
    {
        m_stopping = true;
        m_scheduler.Stop();
    }

    // 모든 producer/worker를 join한 뒤에만 호출한다.
    void Clear()
    {
        m_global.GetJobQueue()->Clear();
        for (auto& channel : m_channels)
        {
            channel->GetJobQueue()->Clear();
            channel->m_pending = 0;
        }

        m_global.m_pending = 0;
        m_pending = 0;
    }

    std::string SnapshotJson() const
    {
        std::ostringstream out;
        out << "{\"schema_version\":1,\"timing_enabled\":" << (wod::core::JobMetrics::Enabled() ? "true" : "false") << ",\"channel_count\":" << m_channels.size() << ",\"max_parallel\":" << m_maxParallel.load()
            << ",\"owner_violations\":" << m_violations.load() << ",\"inbox_pending\":" << m_pending.load() << ",\"global_jobs\":" << m_globalJobs.load() << ",\"global_pending\":" << m_global.m_pending.load()
            << ",\"global_wait_us\":" << m_global.m_waitUs.load() << ",\"global_busy_us\":" << m_global.m_busyUs.load() << ",\"channels\":[";
        bool comma = false;
        for (size_t i = 0; i < m_channels.size(); ++i)
        {
            const auto& channel = *m_channels[i];
            if (!channel.m_jobs.load())
                continue;

            if (comma)
                out << ',';

            comma = true;
            out << "{\"id\":" << i << ",\"jobs\":" << channel.m_jobs.load() << ",\"wait_us\":" << channel.m_waitUs.load() << ",\"max_wait_us\":" << channel.m_maxWaitUs.load() << ",\"busy_us\":" << channel.m_busyUs.load()
                << ",\"pending\":" << channel.m_pending.load() << '}';
        }

        out << "]}";

        return out.str();
    }

  private:
    class Target final : public wod::core::JobTarget
    {
      public:
        Target() : JobTarget(wod::core::JobBudget::Five)
        {
        }

        void OnJobQueueDisconnected() override
        {
        }

        std::atomic_uint64_t m_jobs = 0;
        std::atomic_uint64_t m_waitUs = 0;
        std::atomic_uint64_t m_maxWaitUs = 0;
        std::atomic_uint64_t m_busyUs = 0;
        std::atomic_uint64_t m_pending = 0;
        std::atomic_int m_active = 0;
    };

    static void Maximum(std::atomic_uint64_t& _value, uint64_t _candidate)
    {
        auto old = _value.load();
        while (old < _candidate && !_value.compare_exchange_weak(old, _candidate))
        {
        }
    }

    static uint64_t Timestamp()
    {
        return wod::core::JobMetrics::Enabled() ? wod::core::JobMetrics::NowUs() : 0;
    }

    void ScheduleLocked(SessionWorkQueue& _workQueue)
    {
        const bool global = _workQueue.m_work.front().m_global || _workQueue.m_channel < 0;
        auto* target = global ? &m_global : m_channels.at(_workQueue.m_channel).get();
        _workQueue.m_scheduled = true;
        ++target->m_pending;
        target->GetJobQueue()->PushJob([this, &_workQueue, target, global] {
            Run(_workQueue, *target, global);
        });
        m_scheduler.AddSessionQueue(target);
    }

    void Run(SessionWorkQueue& _workQueue, Target& _target, bool _global)
    {
        Work work;
        {
            std::lock_guard lock(_workQueue.m_mutex);
            work = std::move(_workQueue.m_work.front());
            _workQueue.m_work.pop_front();
            --m_pending;
            --_target.m_pending;
        }

        if (_global)
        {
            std::unique_lock gate(m_world);
            const auto start = Timestamp();
            work.m_execute();
            if (work.m_enqueuedUs)
            {
                m_global.m_waitUs += start - work.m_enqueuedUs;
                m_global.m_busyUs += Timestamp() - start;
            }

            ++m_globalJobs;
        }
        else
        {
            std::shared_lock gate(m_world);
            const auto start = Timestamp();
            if (_target.m_active.fetch_add(1) != 0)
            {
                ++m_violations;
                throw std::logic_error("concurrent channel owners");
            }

            Maximum(m_maxParallel, m_active.fetch_add(1) + 1);
            if (work.m_enqueuedUs)
            {
                const auto wait = start - work.m_enqueuedUs;
                _target.m_waitUs += wait;
                Maximum(_target.m_maxWaitUs, wait);
            }
            // 예외는 worker failure로 서버를 중지한다. 소유권 계수는 항상 복구한다.
            try
            {
                work.m_execute();
            }
            catch (...)
            {
                --m_active;
                --_target.m_active;
                throw;
            }

            --m_active;
            --_target.m_active;
            if (start)
                _target.m_busyUs += Timestamp() - start;

            ++_target.m_jobs;
        }

        std::lock_guard lock(_workQueue.m_mutex);
        _workQueue.m_scheduled = false;
        if (!_workQueue.m_work.empty() && !_workQueue.m_paused && !m_stopping.load())
            ScheduleLocked(_workQueue);
    }

    wod::core::JobScheduler m_scheduler;
    Target m_global;
    std::vector<std::unique_ptr<Target>> m_channels;
    std::shared_mutex m_world;
    std::atomic_bool m_stopping = false;
    std::atomic_uint64_t m_active = 0;
    std::atomic_uint64_t m_maxParallel = 0;
    std::atomic_uint64_t m_violations = 0;
    std::atomic_uint64_t m_globalJobs = 0;
    std::atomic_uint64_t m_pending = 0;
};
}
