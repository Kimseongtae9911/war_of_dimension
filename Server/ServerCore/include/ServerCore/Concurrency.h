#pragma once
#include <ServerCore/Metrics.h>
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <condition_variable>
#include <functional>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace wod::core
{
// producer 그룹을 먼저 join한 뒤 IOCP 그룹을 정리한다. 부분 생성 실패도 같은 경로다.
class ThreadGroup
{
  public:
    using Failure = std::function<void(std::exception_ptr)>;

    ThreadGroup(std::function<void()> _stop, Failure _failure) : m_stop(std::move(_stop)), m_failure(std::move(_failure))
    {
    }

    ~ThreadGroup()
    {
        StopAndJoin();
    }

    template <class Func> void Launch(Func&& _func)
    {
        m_threads.emplace_back([this, work = std::forward<Func>(_func)]() mutable {
            try
            {
                std::invoke(work);
            }
            catch (...)
            {
                m_failed = true;
                m_failure(std::current_exception());
            }
        });
    }

    void StopAndJoin()
    {
        if (m_joined)
            return;

        m_stop();
        for (auto& thread : m_threads)
            if (thread.joinable())
                thread.join();

        m_joined = true;
    }

    bool Failed() const
    {
        return m_failed.load();
    }

  private:
    std::function<void()> m_stop;
    Failure m_failure;
    std::vector<std::thread> m_threads;
    std::atomic_bool m_failed = false;
    bool m_joined = false;
};

class IJob
{
  public:
    virtual ~IJob() = default;
    virtual void Execute() = 0;
    std::atomic_uint64_t m_enqueuedUs = 0;
};

template <class Func> class Job final : public IJob
{
  public:
    explicit Job(Func _f) : m_func(std::move(_f))
    {
    }

    Job(Func _f, std::chrono::high_resolution_clock::time_point _time) : m_func(std::move(_f)), m_time(_time)
    {
    }

    void Execute() override
    {
        std::invoke(m_func);
    }

    bool operator<(const Job& _other) const
    {
        return m_time > _other.m_time;
    }

  private:
    Func m_func;
    std::chrono::high_resolution_clock::time_point m_time = std::chrono::high_resolution_clock::now();
};
enum class JobBudget
{
    Snapshot,
    Five
};

class JobQueue
{
  public:
    using JobRef = std::shared_ptr<IJob>;

    explicit JobQueue(JobBudget _budget) : m_budget(_budget)
    {
    }

    virtual ~JobQueue() = default;

    void PushJob(JobRef _job)
    {
        if (_job)
        {
            if (JobMetrics::Enabled())
                _job->m_enqueuedUs.store(JobMetrics::NowUs(), std::memory_order_relaxed);

            JobMetrics::Submit();
            try
            {
                std::lock_guard lock(m_mutex);
                m_jobs.push(std::move(_job));
            }
            catch (...)
            {
                JobMetrics::Withdraw();
                throw;
            }
        }
    }

    template <class Func>
        requires std::is_invocable_v<std::decay_t<Func>&>
    void PushJob(Func&& _f)
    {
        PushJob(std::make_shared<Job<std::decay_t<Func>>>(std::forward<Func>(_f)));
    }

    virtual void ProcessJob()
    {
        std::lock_guard execute(m_execute);
        size_t count;
        {
            std::lock_guard lock(m_mutex);
            count = m_budget == JobBudget::Five ? size_t{5} : m_jobs.size();
        }

        for (size_t i = 0; i < count; ++i)
        {
            JobRef job;
            {
                std::lock_guard lock(m_mutex);
                if (m_jobs.empty())
                    break;

                job = std::move(m_jobs.front());
                m_jobs.pop();
            }

            JobMetrics::Execution observation(job->m_enqueuedUs.load(std::memory_order_relaxed));
            job->Execute();
            observation.Succeed();
        }
    }

    bool HasJobs() const
    {
        std::lock_guard lock(m_mutex);

        return !m_jobs.empty();
    }

    void Clear()
    {
        std::lock_guard execute(m_execute);
        std::queue<JobRef> discarded;
        {
            std::lock_guard lock(m_mutex);
            m_jobs.swap(discarded);
        }

        while (!discarded.empty())
        {
            discarded.pop();
            JobMetrics::Clear();
        }
    }

  private:
    // 게시 순서를 유지한다. callback은 게시 mutex 밖에서, 실행 mutex 안에서 수행한다.
    std::queue<JobRef> m_jobs;
    mutable std::mutex m_mutex;
    JobBudget m_budget;
    std::mutex m_execute;
};

// 대상은 producer/worker가 종료할 때까지 살아 있어야 한다.
class JobTarget
{
  public:
    explicit JobTarget(JobBudget _budget) : m_jobQueue(_budget)
    {
    }

    virtual ~JobTarget() = default;

    JobQueue* GetJobQueue()
    {
        return &m_jobQueue;
    }

    bool IsDisconnected() const
    {
        return m_isDisconnected.load();
    }

    void SetDisconnected()
    {
        m_isDisconnected = true;
    }

    void ResetDisconnected()
    {
        m_disconnectHandled = false;
        m_isDisconnected = false;
    }

    bool TryBeginDisconnectCleanup()
    {
        bool expected = false;

        return m_disconnectHandled.compare_exchange_strong(expected, true);
    }

    bool TryMarkInQueue()
    {
        bool expected = false;

        return m_isEnqueued.compare_exchange_strong(expected, true);
    }

    void UnmarkInQueue()
    {
        m_isEnqueued = false;
    }

    bool IsInQueue() const
    {
        return m_isEnqueued.load();
    }

    virtual void OnJobQueueDisconnected() = 0;

  private:
    JobQueue m_jobQueue;
    std::atomic_bool m_isDisconnected = false;
    std::atomic_bool m_disconnectHandled = false;
    std::atomic_bool m_isEnqueued = false;
};

class JobScheduler
{
  public:
    void AddSessionQueue(JobTarget* _target)
    {
        std::lock_guard lock(m_mutex);
        if (!m_stopping && _target->TryMarkInQueue())
        {
            m_queue.push(_target);
            m_ready.notify_one();
        }
    }

    void Stop()
    {
        std::lock_guard lock(m_mutex);
        m_stopping = true;
        m_ready.notify_all();
    }

    void ProcessJob()
    {
        for (;;)
        {
            JobTarget* target;
            {
                std::unique_lock lock(m_mutex);
                m_ready.wait(lock, [this] {
                    return m_stopping || !m_queue.empty();
                });
                if (m_stopping)
                    return;

                target = m_queue.front();
                m_queue.pop();
            }
            if (target->IsDisconnected())
            {
                if (target->TryBeginDisconnectCleanup())
                {
                    target->GetJobQueue()->Clear();
                    target->UnmarkInQueue();
                    target->OnJobQueueDisconnected();
                }
                else
                {
                    target->UnmarkInQueue();
                }
            }
            else
            {
                target->GetJobQueue()->ProcessJob();
                target->UnmarkInQueue();
                if (target->GetJobQueue()->HasJobs() || target->IsDisconnected())
                    AddSessionQueue(target);
            }
        }
    }

  private:
    std::mutex m_mutex;
    std::condition_variable m_ready;
    // 한 budget 뒤 남은 대상은 뒤에 등록해 주소에 따른 기아를 막는다.
    std::queue<JobTarget*> m_queue;
    bool m_stopping = false;
};

template <class T> class ObjectPool
{
  public:
    T* Acquire()
    {
        std::lock_guard lock(m_mutex);
        if (m_available.empty())
        {
            auto value = std::make_unique<T>();
            auto* ptr = value.get();
            m_owned.emplace(ptr, std::move(value));
            ++m_leased;

            return ptr;
        }

        T* ptr = m_available.top();
        m_available.pop();
        m_free.erase(ptr);
        ++m_leased;

        return ptr;
    }

    bool try_pop(T*& _ptr)
    {
        std::lock_guard lock(m_mutex);
        if (m_available.empty())
            return false;

        _ptr = m_available.top();
        m_available.pop();
        m_free.erase(_ptr);
        ++m_leased;

        return true;
    }

    void push(T* _ptr)
    {
        if (!_ptr)
            return;

        std::lock_guard lock(m_mutex);
        if (m_free.contains(_ptr))
            throw std::logic_error("I/O pool double return");

        _ptr->Reset();
        if (!m_owned.contains(_ptr))
        {
            m_owned.emplace(_ptr, std::unique_ptr<T>(_ptr));
        }
        else
        {
            if (m_leased == 0)
                throw std::logic_error("I/O pool lease underflow");

            --m_leased;
        }

        m_free.emplace(_ptr, true);
        m_available.push(_ptr);
    }

    size_t Leased() const
    {
        std::lock_guard lock(m_mutex);

        return m_leased;
    }

    size_t Size() const
    {
        std::lock_guard lock(m_mutex);

        return m_owned.size();
    }

    void Clear()
    {
        std::lock_guard lock(m_mutex);
        if (m_leased)
            throw std::logic_error("I/O pool still leased");

        m_available = {};
        m_free.clear();
        m_owned.clear();
    }

  private:
    mutable std::mutex m_mutex;
    std::priority_queue<T*> m_available;
    std::unordered_map<T*, std::unique_ptr<T>> m_owned;
    std::unordered_map<T*, bool> m_free;
    size_t m_leased = 0;
};
}
