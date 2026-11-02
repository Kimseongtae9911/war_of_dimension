#pragma once
#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace wod::core
{
// 연결과 SQL의 소유자는 Run을 호출하는 단일 worker다. 콜백은 mutex 밖에서 호출한다.
class RecoveringWorkQueue
{
  public:
    using Clock = std::chrono::steady_clock;
    enum class Failure
    {
        Expired,
        Stopped,
        Cancelled,
        Execution,
        OutcomeUnknown
    };

    struct Work
    {
        std::function<void()> m_execute;
        std::function<void(Failure)> m_failure;
        bool m_replaySafe = false;
        std::function<bool()> m_valid;
        Clock::time_point m_deadline;
    };

    explicit RecoveringWorkQueue(size_t _capacity = 4096, std::chrono::milliseconds _timeout = std::chrono::seconds(30), std::chrono::milliseconds _initialBackoff = std::chrono::seconds(1),
                                 std::chrono::milliseconds _maxBackoff = std::chrono::seconds(30))
        : m_capacity(_capacity), m_timeout(_timeout), m_initialBackoff(_initialBackoff), m_maxBackoff(_maxBackoff)
    {
        if (!_capacity || _timeout.count() <= 0 || _initialBackoff.count() <= 0 || _maxBackoff < _initialBackoff)
            throw std::invalid_argument("invalid recovery queue limits");
    }

    bool Submit(std::function<void()> _execute, std::function<void(Failure)> _failure = {}, bool _replaySafe = false, std::function<bool()> _valid = {}, bool _persistent = false)
    {
        {
            std::lock_guard lock(m_mutex);
            if (m_stopping || m_outstanding >= m_capacity)
            {
                ++m_rejected;

                return false;
            }

            m_work.push_back({std::move(_execute), std::move(_failure), _replaySafe, std::move(_valid), _persistent ? Clock::time_point::max() : Clock::now() + m_timeout});
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

    void Run(const std::function<bool()>& _reconnect, const std::function<bool()>& _connectionLost, const std::function<void(bool)>& _stateChanged = {})
    {
        auto backoff = m_initialBackoff;
        auto nextAttempt = Clock::now();
        auto lostAt = Clock::now();
        const auto lost = [&] {
            if (m_online.exchange(false))
            {
                ++m_outages;
                lostAt = Clock::now();
                backoff = m_initialBackoff;
                nextAttempt = lostAt + backoff;
                if (_stateChanged)
                    _stateChanged(false);
            }
        };
        for (;;)
        {
            Work work;
            Failure failure = Failure::Execution;
            bool discard = false;
            {
                std::unique_lock lock(m_mutex);
                if (m_stopping && m_work.empty())
                    break;

                const auto now = Clock::now();
                const auto expired = std::find_if(m_work.begin(), m_work.end(), [&](const Work& _work) {
                    return _work.m_deadline <= now || (m_stopping && !m_online);
                });
                if (expired != m_work.end())
                {
                    work = std::move(*expired);
                    m_work.erase(expired);
                    failure = m_stopping ? Failure::Stopped : Failure::Expired;
                    discard = true;
                }
                else if (!m_online)
                {
                    if (now < nextAttempt)
                    {
                        auto wake = nextAttempt;
                        for (const auto& pending : m_work)
                            wake = std::min(wake, pending.m_deadline);

                        m_ready.wait_until(lock, wake);
                        continue;
                    }

                    lock.unlock();
                    ++m_reconnectAttempts;
                    bool connected = false;
                    try
                    {
                        connected = _reconnect();
                    }
                    catch (...)
                    {
                    }

                    if (connected)
                    {
                        m_lastRecoveryMs = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - lostAt).count();
                        ++m_reconnects;
                        m_online = true;
                        if (_stateChanged)
                            _stateChanged(true);
                    }
                    else
                    {
                        backoff = std::min(backoff * 2, m_maxBackoff);
                        nextAttempt = Clock::now() + backoff;
                    }

                    continue;
                }
                else if (m_work.empty())
                {
                    m_ready.wait(lock);
                    continue;
                }
                else
                {
                    work = std::move(m_work.front());
                    m_work.pop_front();
                }
            }

            if (discard)
            {
                Finish(work, failure);
                continue;
            }

            // 정상 경로에는 별도 SQL/연결 진단을 넣지 않는다. 실제 실행 실패 후에만 진단한다.
            if (work.m_deadline <= Clock::now())
            {
                Finish(work, Failure::Expired);
                continue;
            }

            if (work.m_valid && !work.m_valid())
            {
                Finish(work, Failure::Cancelled);
                continue;
            }

            try
            {
                work.m_execute();
                Finish(work);
            }
            catch (...)
            {
                ++m_failed;
                if (_connectionLost())
                {
                    lost();
                    if (work.m_replaySafe)
                    {
                        ++m_retried;
                        Requeue(std::move(work));
                    }
                    else
                    {
                        ++m_unknown;
                        Finish(work, Failure::OutcomeUnknown);
                    }
                }
                else
                {
                    Finish(work, Failure::Execution);
                }
            }
        }
    }

    bool Online() const
    {
        return m_online.load();
    }

    std::string SnapshotJson() const
    {
        std::lock_guard lock(m_mutex);
        std::ostringstream out;
        out << "{\"online\":" << (m_online ? "true" : "false") << ",\"accepted\":" << m_accepted.load() << ",\"completed\":" << m_completed.load() << ",\"rejected\":" << m_rejected.load() << ",\"failed\":" << m_failed.load()
            << ",\"pending\":" << m_outstanding << ",\"outages\":" << m_outages.load() << ",\"reconnect_attempts\":" << m_reconnectAttempts.load() << ",\"reconnects\":" << m_reconnects.load() << ",\"retried\":" << m_retried.load()
            << ",\"outcome_unknown\":" << m_unknown.load() << ",\"expired\":" << m_expired.load() << ",\"cancelled\":" << m_cancelled.load() << ",\"last_recovery_ms\":" << m_lastRecoveryMs.load() << ",\"capacity\":" << m_capacity
            << ",\"timeout_ms\":" << m_timeout.count() << '}';

        return out.str();
    }

  private:
    void Requeue(Work _work)
    {
        std::lock_guard lock(m_mutex);
        m_work.push_front(std::move(_work));
    }

    void Finish(Work& _work, std::optional<Failure> _failure = {})
    {
        if (_failure)
        {
            if (*_failure == Failure::Expired)
                ++m_expired;

            if (*_failure == Failure::Cancelled || *_failure == Failure::Stopped)
                ++m_cancelled;

            try
            {
                if (_work.m_failure)
                    _work.m_failure(*_failure);
            }
            catch (...)
            {
                ++m_failed;
            }
        }

        std::lock_guard lock(m_mutex);
        --m_outstanding;
        ++m_completed;
    }

    const size_t m_capacity;
    const std::chrono::milliseconds m_timeout, m_initialBackoff, m_maxBackoff;
    mutable std::mutex m_mutex;
    std::condition_variable m_ready;
    std::deque<Work> m_work;
    size_t m_outstanding = 0;
    bool m_stopping = false;
    std::atomic_bool m_online = true;
    std::atomic_uint64_t m_accepted = 0, m_completed = 0, m_rejected = 0, m_failed = 0;
    std::atomic_uint64_t m_outages = 0, m_reconnectAttempts = 0, m_reconnects = 0, m_retried = 0, m_unknown = 0, m_expired = 0, m_cancelled = 0, m_lastRecoveryMs = 0;
};
}
