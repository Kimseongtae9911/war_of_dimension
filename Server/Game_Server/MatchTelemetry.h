#pragma once
#include <array>
#include <cstdint>
#include <map>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

namespace wod_server
{
// 기존 매치 업데이트 경로에서 값을 복사한다. reporter는 이 복사본만 읽고 파일 I/O를 수행한다.
class MatchTelemetry
{
  public:
    struct Sample
    {
        int64_t m_timestampMs = 0;
        int64_t m_gameMs = 0;
        int64_t m_nextWaveMs = 0;
        int64_t m_fenceAtGameMs = 0;
        int m_activeMinions = 0;
        bool m_fence = true;
        bool m_finished = false;
        std::array<std::string, 4> m_members;
        std::vector<std::pair<int, int64_t>> m_spawns;
    };

    static void Publish(int _match, Sample _sample)
    {
        std::lock_guard lock(m_mutex);
        _sample.m_members = m_samples[_match].m_members;
        m_samples[_match] = std::move(_sample);
    }

    static void Register(int _match, std::array<std::string, 4> _members)
    {
        std::lock_guard lock(m_mutex);
        m_samples[_match] = {};
        m_samples[_match].m_members = std::move(_members);
    }

    static std::string Json()
    {
        std::map<int, Sample> samples;
        {
            std::lock_guard lock(m_mutex);
            samples = m_samples;
        }

        std::ostringstream json;
        json << "{\"schema_version\":1,\"matches\":[";
        bool first = true;
        for (const auto& [match, sample] : samples)
        {
            if (!first)
                json << ',';

            first = false;
            json << "{\"match_id\":" << match << ",\"timestamp_ms\":" << sample.m_timestampMs << ",\"game_ms\":" << sample.m_gameMs << ",\"next_wave_ms\":" << sample.m_nextWaveMs << ",\"fence_at_game_ms\":" << sample.m_fenceAtGameMs
                 << ",\"active_minions\":" << sample.m_activeMinions << ",\"fence\":" << (sample.m_fence ? "true" : "false") << ",\"finished\":" << (sample.m_finished ? "true" : "false") << ",\"member_keys\":[";
            for (size_t index = 0; index < sample.m_members.size(); ++index)
            {
                if (index)
                    json << ',';

                // 로그인 이름의 byte를 hex로 전송해 JSON escaping/인코딩과 무관하게 매치를 식별한다.
                json << '"';
                for (const unsigned char value : sample.m_members[index])
                    json << "0123456789abcdef"[value >> 4] << "0123456789abcdef"[value & 15];

                json << '"';
            }

            json << "],\"spawns\":[";
            for (size_t index = 0; index < sample.m_spawns.size(); ++index)
            {
                if (index)
                    json << ',';

                json << "{\"id\":" << sample.m_spawns[index].first << ",\"due_ms\":" << sample.m_spawns[index].second << '}';
            }

            json << "]}";
        }

        json << "]}";

        return json.str();
    }

  private:
    inline static std::mutex m_mutex;
    inline static std::map<int, Sample> m_samples;
};
}
