#pragma once
#include <atomic>
#include <chrono>
#include <cstdint>
#include <array>
#include <algorithm>
#include <sstream>
#include <memory>
#include <mutex>
#include <unordered_map>

namespace wod::core
{
// 누적 histogram. reporter의 동시 snapshot은 근사값이며 종료/정지 구간 차분으로 집계한다.
// 0, 1, 2, 4, ... 2^30 us, 마지막 bucket은 초과 값이다.
class MetricHistogram
{
  public:
    void Record(uint64_t _us)
    {
        size_t bucket = 0;
        uint64_t bound = 0;
        while (bucket < 32 && _us > bound)
        {
            ++bucket;
            bound = bucket == 1 ? 1 : bound * 2;
        }

        m_buckets[bucket].fetch_add(1, std::memory_order_relaxed);
        m_sumUs.fetch_add(_us, std::memory_order_relaxed);
        auto maximum = m_maxUs.load(std::memory_order_relaxed);
        while (maximum < _us && !m_maxUs.compare_exchange_weak(maximum, _us, std::memory_order_relaxed))
        {
        }
    }

    std::string Json() const
    {
        std::ostringstream json;
        json << "{\"buckets\":[";
        for (size_t index = 0; index < m_buckets.size(); ++index)
        {
            if (index)
                json << ',';

            json << m_buckets[index].load(std::memory_order_relaxed);
        }

        json << "],\"sum_us\":" << m_sumUs.load(std::memory_order_relaxed) << ",\"max_us\":" << m_maxUs.load(std::memory_order_relaxed) << '}';

        return json.str();
    }

  private:
    std::array<std::atomic_uint64_t, 33> m_buckets{};
    std::atomic_uint64_t m_sumUs = 0, m_maxUs = 0;
};

// 실행 중 enable 상태를 바꾸지 않는다. 서버 Run 시작/worker join 이후에만 변경한다.
class JobMetrics
{
  public:
    using Clock = std::chrono::steady_clock;
    inline static std::atomic_bool m_enabled = false;
    inline static std::atomic_uint64_t m_submitted = 0, m_started = 0, m_completed = 0, m_failed = 0, m_cleared = 0, m_running = 0, m_executeUs = 0, m_maxExecuteUs = 0;
    inline static std::atomic_uint64_t m_packets = 0, m_moves = 0, m_stops = 0;
    inline static MetricHistogram m_wait, m_packetWait, m_tickDelay, m_tickDuration, m_lobbyGap;
    inline static MetricHistogram m_tickTimerLateness, m_tickDispatchWait;
    inline static std::mutex m_packetMutex;
    inline static std::unordered_map<const void *, uint64_t> m_pendingPackets;
    inline static std::atomic_uint64_t m_packetsDiscarded = 0;

    // 패킷 작업만 추적한다. 자기 재등록 이동 작업의 높은 빈도와 미실행 작업의 대기를 분리한다.
    class PendingPacket
    {
      public:
        PendingPacket() : m_enqueuedUs(NowUs())
        {
            std::lock_guard lock(m_packetMutex);
            m_pendingPackets.emplace(this, m_enqueuedUs);
        }

        ~PendingPacket()
        {
            std::lock_guard lock(m_packetMutex);
            if (m_pendingPackets.erase(this))
                m_packetsDiscarded.fetch_add(1, std::memory_order_relaxed);
        }

        void Start()
        {
            bool removed;
            {
                std::lock_guard lock(m_packetMutex);
                removed = m_pendingPackets.erase(this) != 0;
            }
            if (removed)
                m_packetWait.Record(NowUs() - m_enqueuedUs);
        }

      private:
        uint64_t m_enqueuedUs;
    };

    static std::shared_ptr<PendingPacket> TrackPacket()
    {
        return Enabled() ? std::make_shared<PendingPacket>() : nullptr;
    }

    static std::string PendingPacketJson()
    {
        std::lock_guard lock(m_packetMutex);
        const auto now = NowUs();
        uint64_t oldest = 0;
        for (const auto& [key, enqueued] : m_pendingPackets)
            oldest = (std::max)(oldest, now - enqueued);

        return std::string("{\"count\":") + std::to_string(m_pendingPackets.size()) + ",\"oldest_wait_us\":" + std::to_string(oldest) + ",\"discarded\":" + std::to_string(m_packetsDiscarded.load()) + '}';
    }

    static uint64_t NowUs()
    {
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(Clock::now().time_since_epoch()).count());
    }

    static bool Enabled()
    {
        return m_enabled.load(std::memory_order_relaxed);
    }

    static void Packet()
    {
        if (Enabled())
            m_packets.fetch_add(1, std::memory_order_relaxed);
    }

    static void Move(bool _stop)
    {
        if (Enabled())
            (_stop ? m_stops : m_moves).fetch_add(1, std::memory_order_relaxed);
    }

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
        explicit Execution(uint64_t _enqueuedUs = 0) : m_enabled(JobMetrics::m_enabled.load(std::memory_order_relaxed))
        {
            if (m_enabled)
            {
                m_start = Clock::now();
                if (_enqueuedUs)
                    m_wait.Record(NowUs() - _enqueuedUs);

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

    class Tick
    {
      public:
        Tick() : m_start(Enabled() ? NowUs() : 0)
        {
        }

        ~Tick()
        {
            if (m_start)
                m_tickDuration.Record(NowUs() - m_start);
        }

      private:
        uint64_t m_start;
    };
};
}
