#include <ServerCore/Telemetry.h>
#include <ServerCore/Metrics.h>
#include <Psapi.h>
#include <fstream>
#include <iostream>
#include <sstream>
#pragma comment(lib, "Psapi.lib")

namespace wod::core
{
namespace
{
uint64_t EpochMs()
{
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

uint64_t Ticks(const FILETIME& _value)
{
    return (static_cast<uint64_t>(_value.dwHighDateTime) << 32) | _value.dwLowDateTime;
}
}

TelemetryReporter::TelemetryReporter(const char *_role, std::function<uint64_t()> _leased, std::function<std::string()> _content)
    : m_role(_role), m_leased(std::move(_leased)), m_content(std::move(_content)), m_startedMs(EpochMs()), m_start(std::chrono::steady_clock::now())
{
    const auto size = GetEnvironmentVariableW(L"WOD_METRICS_DIRECTORY", nullptr, 0);
    if (!size)
        return;

    std::wstring directory(size, L'\0');
    const auto length = GetEnvironmentVariableW(L"WOD_METRICS_DIRECTORY", directory.data(), size);
    if (!length || length >= size)
        throw std::runtime_error("invalid metrics directory");

    directory.resize(length);
    m_output = std::filesystem::path(directory) / (m_role + ".json");
    JobMetrics::m_enabled = true;
    try
    {
        m_thread = std::thread([this] {
            Run();
        });
    }
    catch (...)
    {
        JobMetrics::m_enabled = false;
        throw;
    }
}

TelemetryReporter::~TelemetryReporter()
{
    if (!m_thread.joinable())
        return;

    {
        std::lock_guard lock(m_mutex);
        m_stopping = true;
        m_ready.notify_all();
    }
    m_thread.join();
    JobMetrics::m_enabled = false;
}

void TelemetryReporter::Run()
{
    bool reported = false;
    for (;;)
    {
        bool stopped;
        {
            std::lock_guard lock(m_mutex);
            stopped = m_stopping;
        }
        try
        {
            Sample(stopped);
        }
        catch (const std::exception& error)
        {
            if (!reported)
                std::cerr << "Telemetry export failed: " << error.what() << '\n';

            reported = true;
        }
        if (stopped)
            return;

        std::unique_lock lock(m_mutex);
        m_ready.wait_for(lock, std::chrono::seconds(1), [this] {
            return m_stopping;
        });
    }
}

void TelemetryReporter::Sample(bool _stopped)
{
    const auto sampleStart = std::chrono::steady_clock::now();
    const auto stats = NetworkRuntime::Get().Stats();
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS *>(&memory), sizeof(memory)))
        throw std::runtime_error("GetProcessMemoryInfo failed");

    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
        throw std::runtime_error("GetProcessTimes failed");

    const auto submitted = JobMetrics::m_submitted.load();
    const auto started = JobMetrics::m_started.load();
    const auto cleared = JobMetrics::m_cleared.load();
    std::ostringstream json;
    json << "{\"schema_version\":1,\"role\":\"" << m_role << "\",\"pid\":" << GetCurrentProcessId() << ",\"instance\":\"" << GetCurrentProcessId() << '-' << m_startedMs << "\",\"sequence\":" << ++m_sequence << ",\"timestamp_ms\":" << EpochMs()
         << ",\"uptime_ms\":" << std::chrono::duration_cast<std::chrono::milliseconds>(sampleStart - m_start).count() << ",\"stopped\":" << (_stopped ? "true" : "false") << ",\"cpu_ticks\":" << Ticks(kernel) + Ticks(user)
         << ",\"logical_processors\":" << (std::max)(1u, std::thread::hardware_concurrency()) << ",\"private_bytes\":" << memory.PrivateUsage << ",\"working_set_bytes\":" << memory.WorkingSetSize << ",\"pending_io\":" << stats.m_pending
         << ",\"received_bytes\":" << stats.m_received << ",\"sent_bytes\":" << stats.m_sent << ",\"io_errors\":" << stats.m_errors << ",\"sockets\":" << stats.m_sockets << ",\"leased\":" << m_leased() << ",\"jobs_submitted\":" << submitted
         << ",\"jobs_started\":" << started << ",\"jobs_cleared\":" << cleared << ",\"jobs_queued\":" << (submitted > started + cleared ? submitted - started - cleared : 0) << ",\"jobs_running\":" << JobMetrics::m_running.load()
         << ",\"jobs_completed\":" << JobMetrics::m_completed.load() << ",\"jobs_failed\":" << JobMetrics::m_failed.load() << ",\"job_execute_us\":" << JobMetrics::m_executeUs.load() << ",\"job_max_execute_us\":" << JobMetrics::m_maxExecuteUs.load();
    if (m_content)
        json << ",\"content\":" << m_content();

    json << ",\"collect_us\":" << std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - sampleStart).count() << "}\n";
    std::filesystem::create_directories(m_output.parent_path());
    auto temporary = m_output;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.exceptions(std::ios::failbit | std::ios::badbit);
        file << json.str();
        file.close();
    }
    if (!MoveFileExW(temporary.c_str(), m_output.c_str(), MOVEFILE_REPLACE_EXISTING))
        throw std::runtime_error("metrics snapshot replace failed");
}
}
