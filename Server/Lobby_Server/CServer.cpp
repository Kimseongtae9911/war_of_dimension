#include "pch.h"
#include <ServerCore/Session.h>
#include <ServerCore/Telemetry.h>
#include <ServerCore/TickProfile.h>
#include "CServer.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CPacketMgr.h"
#include "CUserMgr.h"

//#define Test

namespace wod_server
{

bool CServer::Initialize()
{
    LogPrinter::PrintMsg("Server Initialize Start");
    LogPrinter::PrintMsg("Lobby channel capacity: ", LOBBY_CHANNEL_CAPACITY);
    NetworkRuntime::Start();
    m_transportReady = true;

    if (!network::GetInstance()->Initialize())
    {
        LogPrinter::PrintMsg("NetworkMgr Initialize Fail");
        return false;
    }

    if (!match::GetInstance()->Initialize())
    {
        LogPrinter::PrintMsg("MatchMgr Initialize Fail");
        return false;
    }
    m_matchReady = true;

    if (!CPacketMgr::GetInstance()->Initialize())
    {
        LogPrinter::PrintMsg("PacketMgr Initialize Fail");
        return false;
    }
    m_packetReady = true;

    if (!CUserMgr::GetInstance()->Initialize())
    {
        LogPrinter::PrintMsg("UserMgr Initialize Fail");
        return false;
    }

    if (!GameUtil::LoadNaviMesh("Resource/NavMeshData2.obj"))
    {
        LogPrinter::PrintMsg("Failed To Load NaviMesh");
        return false;
    }

    if (!GameUtil::LoadHeightMesh("Resource/HeightMesh2.obj"))
    {
        LogPrinter::PrintMsg("Failed To Load HeightMesh");
        return false;
    }

    if (!GameUtil::LoadMap("Resource/Map.txt"))
    {
        LogPrinter::PrintMsg("Failed To Load Map");
        return false;
    }

    LogPrinter::PrintMsg("Server Initialize Finish");
    return true;
}

bool CServer::Release()
{
    if (!m_transportReady)
        return true;
    bool ok = true;
    ok = network::GetInstance()->Release() && ok;
    if (std::exchange(m_matchReady, false))
        ok = match::GetInstance()->Release() && ok;
    if (std::exchange(m_packetReady, false))
        ok = CPacketMgr::GetInstance()->Release() && ok;
    ok = CUserMgr::GetInstance()->Release() && ok;
    NetworkRuntime::Stop();
    m_transportReady = false;
    return ok;
}

void CServer::Run()
{
    wchar_t profileFlag[2]{};
    wod::core::TickProfile::Enable(GetEnvironmentVariableW(L"WOD_TICK_PROFILE", profileFlag, 2) == 1 && profileFlag[0] == L'1');
    wod::core::ProcessStopSignal signal;
    const std::string terrainStats = GameUtil::TerrainIndexJson();
    wod::core::TelemetryReporter telemetry(
        "LobbyServer",
        [] {
            return Resource::m_overExPool.Leased();
        },
        [terrainStats] {
            return std::string("{\"channel_capacity\":") + std::to_string(LOBBY_CHANNEL_CAPACITY) + ",\"storage_capacity\":" + std::to_string(MAX_CLIENT) +
                   ",\"movement_interval_us\":" + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(CClient::m_movementInterval).count()) + ",\"movement_single_pending\":true,\"logic_workers\":" + std::to_string(LOBBY_WORKERS) +
                   ",\"database\":" + network::GetInstance()->GetDataBaseThread()->SnapshotJson() + ",\"dispatcher\":" + GLobbyDispatcher->SnapshotJson() + ",\"terrain_index\":" + terrainStats + '}';
        });
    const unsigned int count = 4u;
    const auto failure = [](std::exception_ptr _error) {
        try
        {
            std::rethrow_exception(_error);
        }
        catch (const std::exception& detail)
        {
            LogPrinter::PrintMsg(std::string("Worker failed: ") + detail.what());
        }
        catch (...)
        {
            LogPrinter::PrintMsg("Worker failed: unknown exception");
        }
        wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
    };
    wod::core::ThreadGroup iocp(
        [&] {
            network::GetInstance()->PrepareStop();
            NetworkRuntime::Get().RequestStop(count);
        },
        failure);
    wod::core::ThreadGroup producers(
        [] {
            network::GetInstance()->PrepareStop();
            GLobbyDispatcher->Stop();
        },
        failure);
    for (unsigned int i = 0; i < count; ++i)
        iocp.Launch([] {
            network::GetInstance()->IOCPFunc();
        });
    producers.Launch([] {
        network::GetInstance()->TimerFunc();
    });
    for (int i = 0; i < LOBBY_WORKERS; ++i)
        producers.Launch([] {
            GLobbyDispatcher->ProcessJob();
        });
    signal.Wait();
    producers.StopAndJoin();
    iocp.StopAndJoin();
    network::GetInstance()->DataBaseFunc(); // producer join 후 수락 작업 drain, 객체 해제 전 완료
    LogPrinter::PrintMsg("Lobby dispatcher stop " + GLobbyDispatcher->SnapshotJson());
    GLobbyDispatcher->Clear();
    CUserMgr::GetInstance()->ClearWorkQueues();
    if (producers.Failed() || iocp.Failed() || network::GetInstance()->WorkerFailed())
        throw std::runtime_error("server worker failure");
}

}
