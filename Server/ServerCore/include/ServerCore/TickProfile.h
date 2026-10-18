#pragma once
#include <WinSock2.h>
#include <Windows.h>
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <sstream>

namespace wod::core
{
// 선택 활성화 진단. worker의 tick 안에서만 수집하고 tick 종료 때 누적한다.
// root와 detail은 포함 관계다. 서로 합산해 전체 시간으로 해석하지 않는다.
class TickProfile
{
  public:
    enum class Stage : size_t
    {
        Total,
        Clock,
        Jobs,
        PlayerUpdate,
        PlayerSend,
        MinionUpdate,
        MonsterUpdate,
        Periodic,
        Observation,
        PlayerMove,
        MapCollision,
        NavRay,
        HeightRay,
        ClientCollision,
        NpcCollision,
        Astar,
        SendLock,
        SendBody,
        NativeSend,
        PlayerHeight,
        MinionHeight,
        MonsterHeight,
        LobbyNav,
        LobbyHeight,
        LobbyView,
        Count
    };
    inline static constexpr size_t m_stageCount = static_cast<size_t>(Stage::Count);
    inline static constexpr std::array<const char *, m_stageCount> m_names = {"total",       "game_clock",  "jobs",          "player_update",     "player_send",       "minion_update",     "monster_update", "periodic",
                                                                              "observation", "player_move", "map_collision", "nav_ray",           "height_ray",        "client_collision",  "npc_collision",  "astar",
                                                                              "send_lock",   "send_body",   "native_send",   "height_ray_player", "height_ray_minion", "height_ray_monster",
                                                                              "lobby_nav", "lobby_height", "lobby_view"};

    struct Value
    {
        uint64_t m_calls = 0, m_wallNs = 0, m_cycles = 0, m_cycleSamples = 0, m_cycleErrors = 0, m_work = 0;
    };

    struct Counter
    {
        std::atomic_uint64_t m_calls = 0, m_wallNs = 0, m_cycles = 0, m_cycleSamples = 0, m_cycleErrors = 0, m_work = 0;
    };

    using Clock = std::chrono::steady_clock;

    static void Enable(bool _enabled)
    {
        // worker 시작 전/종료 후에만 변경한다. 프로세스 재시작으로 누적값을 초기화한다.
        m_enabled.store(_enabled, std::memory_order_relaxed);
    }

    static bool Enabled()
    {
        return m_enabled.load(std::memory_order_relaxed);
    }

    class Scope;

    class Frame
    {
      public:
        Frame() : m_previous(m_current), m_active(Enabled())
        {
            if (m_active)
            {
                m_current = this;
                m_start = Clock::now();
                m_cycleValid = QueryThreadCycleTime(GetCurrentThread(), &m_cycles) != 0;
            }
        }

        ~Frame()
        {
            if (!m_active)
                return;

            ULONG64 cycles = 0;
            const bool valid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != 0 && m_cycleValid && cycles >= m_cycles;
            auto& total = m_values[static_cast<size_t>(Stage::Total)];
            total.m_calls = 1;
            total.m_wallNs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_start).count());
            total.m_cycles = valid ? cycles - m_cycles : 0;
            total.m_cycleSamples = valid ? 1 : 0;
            total.m_cycleErrors = valid ? 0 : 1;
            m_current = m_previous;
            for (size_t index = 0; index < m_stageCount; ++index)
            {
                const auto& value = m_values[index];
                if (!value.m_calls)
                    continue;

                auto& counter = m_counters[index];
                counter.m_calls.fetch_add(value.m_calls, std::memory_order_relaxed);
                counter.m_wallNs.fetch_add(value.m_wallNs, std::memory_order_relaxed);
                counter.m_cycles.fetch_add(value.m_cycles, std::memory_order_relaxed);
                counter.m_cycleSamples.fetch_add(value.m_cycleSamples, std::memory_order_relaxed);
                counter.m_cycleErrors.fetch_add(value.m_cycleErrors, std::memory_order_relaxed);
                counter.m_work.fetch_add(value.m_work, std::memory_order_relaxed);
            }
        }

        Frame(const Frame&) = delete;
        Frame& operator=(const Frame&) = delete;

      private:
        friend class Scope;
        std::array<Value, m_stageCount> m_values{};
        Stage m_owner = Stage::Total;
        Frame *m_previous;
        bool m_active, m_cycleValid = false;
        Clock::time_point m_start{};
        ULONG64 m_cycles = 0;
    };

    class Scope
    {
      public:
        explicit Scope(Stage _stage) : m_frame(m_current), m_stage(_stage)
        {
            if (m_frame)
            {
                m_previousOwner = m_frame->m_owner;
                if (_stage == Stage::PlayerUpdate || _stage == Stage::MinionUpdate || _stage == Stage::MonsterUpdate)
                    m_frame->m_owner = _stage;

                m_start = Clock::now();
                m_cycleValid = QueryThreadCycleTime(GetCurrentThread(), &m_cycles) != 0;
            }
        }

        ~Scope()
        {
            if (!m_frame)
                return;

            ULONG64 cycles = 0;
            const bool valid = QueryThreadCycleTime(GetCurrentThread(), &cycles) != 0 && m_cycleValid && cycles >= m_cycles;
            const auto wallNs = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - m_start).count());
            const auto record = [&](Stage _stage) {
                auto& value = m_frame->m_values[static_cast<size_t>(_stage)];
                ++value.m_calls;
                value.m_wallNs += wallNs;
                value.m_cycles += valid ? cycles - m_cycles : 0;
                value.m_cycleSamples += valid ? 1 : 0;
                value.m_cycleErrors += valid ? 0 : 1;
                value.m_work += m_work;
            };
            record(m_stage);
            if (m_stage == Stage::HeightRay)
            {
                if (m_frame->m_owner == Stage::PlayerUpdate)
                {
                    record(Stage::PlayerHeight);
                }
                else if (m_frame->m_owner == Stage::MinionUpdate)
                {
                    record(Stage::MinionHeight);
                }
                else if (m_frame->m_owner == Stage::MonsterUpdate)
                {
                    record(Stage::MonsterHeight);
                }
            }

            m_frame->m_owner = m_previousOwner;
        }

        void Work(uint64_t _work)
        {
            m_work = _work;
        }

        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;

      private:
        Frame *m_frame;
        Stage m_stage;
        Stage m_previousOwner = Stage::Total;
        Clock::time_point m_start{};
        ULONG64 m_cycles = 0;
        uint64_t m_work = 0;
        bool m_cycleValid = false;
    };

    static std::string Json()
    {
        std::ostringstream json;
        json << "{\"schema_version\":1,\"enabled\":" << (Enabled() ? "true" : "false") << ",\"stages\":{";
        for (size_t index = 0; index < m_stageCount; ++index)
        {
            if (index)
                json << ',';

            const auto& value = m_counters[index];
            json << '"' << m_names[index] << "\":{\"calls\":" << value.m_calls.load() << ",\"wall_ns\":" << value.m_wallNs.load() << ",\"cycles\":" << value.m_cycles.load() << ",\"cycle_samples\":" << value.m_cycleSamples.load()
                 << ",\"cycle_errors\":" << value.m_cycleErrors.load() << ",\"work\":" << value.m_work.load() << '}';
        }

        json << "}}";

        return json.str();
    }

  private:
    inline static std::atomic_bool m_enabled = false;
    inline static thread_local Frame *m_current = nullptr;
    inline static std::array<Counter, m_stageCount> m_counters;
};
}
