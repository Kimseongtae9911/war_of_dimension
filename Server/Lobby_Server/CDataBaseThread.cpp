#include "pch.h"
#include "CDataBaseThread.h"
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
            m_database = &database;
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

void CDataBaseThread::Stop()
{
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
           ",\"failed\":" + std::to_string(m_queue.Failed()) + '}';
}
}
