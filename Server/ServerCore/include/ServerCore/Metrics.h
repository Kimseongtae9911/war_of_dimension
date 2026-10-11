#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>

namespace wod::core
{
// 실행 중 enable 상태를 바꾸지 않는다. 서버 Run 시작/worker join 이후에만 변경한다.
class JobMetrics
{
  public:
    using Clock = std::chrono::steady_clock;
    inline static std::atomic_bool m_enabled = false;
    inline static std::atomic_uint64_t m_submitted = 0, m_started = 0, m_completed = 0, m_failed = 0, m_cleared = 0, m_running = 0, m_executeUs = 0, m_maxExecuteUs = 0;

    static void Submit()
    {
        if (m_enabled.load(std::memory_order_relaxed))
            m_submitted.fetch_add(1, std::memory_order_relaxed);
    }

    static void Withdraw()
    {
        if (m_enabled.load(std::memory_order_relaxed))
            m_submitted.fetch_sub(1, std::memory_order_relaxed);
    }

    static void Clear()
    {
        if (m_enabled.load(std::memory_order_relaxed))
            m_cleared.fetch_add(1, std::memory_order_relaxed);
    }

    class Execution
    {
      public:
        Execution() : m_enabled(JobMetrics::m_enabled.load(std::memory_order_relaxed))
        {
            if (m_enabled)
            {
                m_start = Clock::now();
                m_started.fetch_add(1, std::memory_order_relaxed);
                m_running.fetch_add(1, std::memory_order_relaxed);
            }
        }

        ~Execution()
        {
            if (!m_enabled)
                return;

            const auto elapsed = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now() - m_start).count());
            m_executeUs.fetch_add(elapsed, std::memory_order_relaxed);
            auto maximum = m_maxExecuteUs.load(std::memory_order_relaxed);
            while (maximum < elapsed && !m_maxExecuteUs.compare_exchange_weak(maximum, elapsed, std::memory_order_relaxed))
            {
            }

            (m_succeeded ? m_completed : m_failed).fetch_add(1, std::memory_order_relaxed);
            m_running.fetch_sub(1, std::memory_order_relaxed);
        }

        void Succeed()
        {
            m_succeeded = true;
        }

      private:
        bool m_enabled, m_succeeded = false;
        Clock::time_point m_start{};
    };
};
}
