#pragma once
#include <ServerCore/Net.h>
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <mutex>
#include <thread>

namespace wod::core
{
// WOD_METRICS_DIRECTORY가 지정된 실행에서만 활성화한다. callback 대상은 이 객체보다 오래 살아야 한다.
class TelemetryReporter
{
  public:
    TelemetryReporter(const char *_role, std::function<uint64_t()> _leased, std::function<std::string()> _content = {});
    ~TelemetryReporter();
    TelemetryReporter(const TelemetryReporter&) = delete;
    TelemetryReporter& operator=(const TelemetryReporter&) = delete;

  private:
    void Run();
    void Sample(bool _stopped);
    std::filesystem::path m_output;
    std::string m_role;
    std::function<uint64_t()> m_leased;
    std::function<std::string()> m_content;
    std::thread m_thread;
    std::mutex m_mutex;
    std::condition_variable m_ready;
    bool m_stopping = false;
    uint64_t m_sequence = 0, m_startedMs = 0;
    std::chrono::steady_clock::time_point m_start;
};
}
