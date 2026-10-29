#include "pch.h"
#include "CDataBaseThread.h"
#include "CUserMgr.h"
#include <filesystem>
#include <future>

namespace wod_server
{
namespace
{
std::wstring ReadConnection()
{
    const DWORD size = GetEnvironmentVariableW(L"WOD_DB_CONNECTION", nullptr, 0);
    if (size)
    {
        std::wstring value(size, L'\0');
        GetEnvironmentVariableW(L"WOD_DB_CONNECTION", value.data(), size);
        value.resize(size - 1);

        return value;
    }

    wchar_t file[32768]{};
    if (!GetEnvironmentVariableW(L"WOD_DB_CONFIG", file, static_cast<DWORD>(std::size(file))))
        throw std::runtime_error("DB mode requires WOD_DB_CONNECTION or WOD_DB_CONFIG; run scripts/Initialize-LocalDatabase.ps1");

    std::wifstream input{std::filesystem::path(file)};
    std::wstring value;
    if (!std::getline(input, value) || value.empty())
        throw std::runtime_error("Cannot read DB connection configuration");

    return value;
}
}

CDataBaseThread::CDataBaseThread()
{
    wchar_t mode[32]{};
    const auto size = GetEnvironmentVariableW(L"WOD_DB_MODE", mode, static_cast<DWORD>(std::size(mode)));
    if (!size || std::wstring_view(mode) == L"disabled")
        return;

    if (size >= std::size(mode) || std::wstring_view(mode) != L"odbc")
        throw std::runtime_error("WOD_DB_MODE must be disabled or odbc");

    const auto connection = ReadConnection();
    std::promise<void> ready;
    auto result = ready.get_future();
    m_worker = std::thread([this, connection, &ready] {
        CDataBase database;
        try
        {
            if (!database.Connect(connection))
                throw std::runtime_error("DB connection failed; check driver, server and database configuration");

            database.ValidateSchema();
            const auto identity = database.JournalNamespace();
            const auto key = RewardJournal::Hash(identity.data(), identity.size() * sizeof(wchar_t));
            wchar_t custom[32768]{};
            std::filesystem::path directory;
            const auto length = GetEnvironmentVariableW(L"WOD_DB_REWARD_DIRECTORY", custom, static_cast<DWORD>(std::size(custom)));
            if (length >= std::size(custom))
                throw std::runtime_error("reward directory is too long");

            if (length)
            {
                directory = custom;
            }
            else
            {
                wchar_t executable[32768]{};
                const auto size = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(std::size(executable)));
                if (!size || size >= std::size(executable))
                    throw std::runtime_error("cannot locate reward storage");

                directory = std::filesystem::path(executable).parent_path();
                for (int i = 0; i < 4; ++i)
                    directory = directory.parent_path();

                directory /= L".runtime/database-rewards";
            }

            m_rewards = std::make_unique<RewardJournal>(directory / key);
            m_database = &database;
            for (auto& record : m_rewards->Recover())
                ScheduleReward(std::move(record));

            ready.set_value();
        }
        catch (...)
        {
            ready.set_exception(std::current_exception());

            return;
        }

        m_queue.Run();
        m_database = nullptr;
    });
    try
    {
        result.get();
        try
        {
            m_rewardWriter = std::thread([this] {
                m_rewardIngress.Run();
            });
        }
        catch (...)
        {
            m_queue.StopAccepting();
            throw;
        }

        m_enabled = true;
    }
    catch (...)
    {
        m_worker.join();
        throw;
    }
}

CDataBaseThread::~CDataBaseThread()
{
    Stop();
}

bool CDataBaseThread::Submit(std::function<void(CDataBase&)> _work)
{
    if (!m_enabled)
        return false;

    return m_queue.Submit([this, work = std::move(_work)] {
        m_database->ResetRequest();
        try
        {
            work(*m_database);
            m_database->ThrowIfFailed();
        }
        catch (...)
        {
            m_database->ResetRequest();
            throw;
        }

        m_database->ResetRequest();
    });
}

void CDataBaseThread::SubmitReward(const std::string& _account, int _delta)
{
    if (!m_enabled)
        throw std::runtime_error("reward submitted while database disabled");

    // 로비에서는 값만 제출한다. 별도 writer가 디스크 확정 후 SQL 큐로 넘긴다.
    if (!m_rewardIngress.Submit([this, account = _account, delta = _delta] {
            try
            {
                ScheduleReward(m_rewards->Append(account, delta));
            }
            catch (...)
            {
                LogPrinter::PrintMsg("Reward recording failed: stopping for recovery inspection");
                wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
                throw;
            }
        }))
        throw std::runtime_error("reward ingress queue rejected; no successful persistence acknowledgement");
}

void CDataBaseThread::ScheduleReward(RewardJournal::Record _record)
{
    const bool accepted = m_queue.Submit([this, record = std::move(_record)] {
        m_database->ResetRequest();
        try
        {
            m_database->ApplyReward(record.m_operation, record.m_account, record.m_delta);
            const auto balance = m_database->ReadBalance(record.m_account);
            m_database->ThrowIfFailed();
            m_rewards->Complete(record);
            GLobbyDispatcher->PostGlobal([account = record.m_account, balance] {
                CUserMgr::GetInstance()->PublishBalance(account, balance);
            });
        }
        catch (...)
        {
            m_database->ResetRequest();
            LogPrinter::PrintMsg("Reward pending: recovery record retained; replay on next startup");
            throw;
        }

        m_database->ResetRequest();
    });
    if (!accepted)
        LogPrinter::PrintMsg("Reward queue full or stopping: recovery record retained");
}

void CDataBaseThread::Stop()
{
    m_rewardIngress.StopAccepting();
    if (m_rewardWriter.joinable())
        m_rewardWriter.join();

    m_queue.StopAccepting();
    if (m_worker.joinable())
    {
        m_worker.join();
        LogPrinter::PrintMsg("Database drain " + SnapshotJson());
    }
}

std::string CDataBaseThread::SnapshotJson() const
{
    return std::string("{\"enabled\":") + (m_enabled ? "true" : "false") + ",\"accepted\":" + std::to_string(m_queue.Accepted()) + ",\"completed\":" + std::to_string(m_queue.Completed()) + ",\"rejected\":" + std::to_string(m_queue.Rejected()) +
           ",\"failed\":" + std::to_string(m_queue.Failed()) + ",\"reward_pending\":" + std::to_string(m_rewards ? m_rewards->Pending() : 0) + ",\"reward_ingress_accepted\":" + std::to_string(m_rewardIngress.Accepted()) +
           ",\"reward_ingress_completed\":" + std::to_string(m_rewardIngress.Completed()) + ",\"reward_ingress_failed\":" + std::to_string(m_rewardIngress.Failed()) + ",\"reward_ingress_rejected\":" + std::to_string(m_rewardIngress.Rejected()) + '}';
}
}
