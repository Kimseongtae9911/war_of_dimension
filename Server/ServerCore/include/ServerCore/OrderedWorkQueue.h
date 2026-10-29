#pragma once
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <stdexcept>

namespace wod::core
{
// 수락 시점의 mutex 획득 순서가 실행 순서다. Run은 한 worker만 호출한다.
class OrderedWorkQueue
{
  public:
    explicit OrderedWorkQueue(size_t _capacity = 4096) : m_capacity(_capacity)
    {
        if (!_capacity)
            throw std::invalid_argument("queue capacity must be positive");
    }

    bool Submit(std::function<void()> _work)
    {
        {
            std::lock_guard lock(m_mutex);
            if (m_stopping || m_outstanding >= m_capacity)
            {
                ++m_rejected;

                return false;
            }

            m_work.push_back(std::move(_work));
            ++m_outstanding;
            ++m_accepted;
        }

        m_ready.notify_one();

        return true;
    }

    void StopAccepting()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stopping = true;
        }

        m_ready.notify_all();
    }

    void Run()
    {
        for (;;)
        {
            std::function<void()> work;
            {
                std::unique_lock lock(m_mutex);
                m_ready.wait(lock, [this] {
                    return m_stopping || !m_work.empty();
                });
                if (m_work.empty())
                    break;

                work = std::move(m_work.front());
                m_work.pop_front();
            }

            try
            {
                work();
            }
            catch (...)
            {
                ++m_failed;
            }

            {
                std::lock_guard lock(m_mutex);
                --m_outstanding;
                ++m_completed;
            }
        }
    }

    uint64_t Accepted() const
    {
        return m_accepted.load();
    }

    uint64_t Completed() const
    {
        return m_completed.load();
    }

    uint64_t Rejected() const
    {
        return m_rejected.load();
    }

    uint64_t Failed() const
    {
        return m_failed.load();
    }

  private:
    const size_t m_capacity;
    std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<std::function<void()>> m_work;
    size_t m_outstanding = 0;
    bool m_stopping = false;
    std::atomic_uint64_t m_accepted = 0;
    std::atomic_uint64_t m_completed = 0;
    std::atomic_uint64_t m_rejected = 0;
    std::atomic_uint64_t m_failed = 0;
};
}
