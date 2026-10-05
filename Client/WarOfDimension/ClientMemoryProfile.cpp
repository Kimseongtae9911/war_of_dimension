#include "stdafx.h"
#include "ClientMemoryProfile.h"
#include "GameFramework.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include <psapi.h>
#include <filesystem>
#include <cstdint>
#include <stdexcept>
#pragma comment(lib, "psapi.lib")

namespace
{
    bool active = false, legacyGeometry = false;
    bool heroMeasurement = false, fullHeroParts = false;
    bool particleMeasurement = false, fullParticlePool = false, particleReleaseChecked = false;
    bool reuseMeasurement = false, dedicatedBuffers = false;
    std::uint64_t heroLoads = 0, heroSkins = 0, heroAnimatedFrames = 0, heroMatrixBytes = 0;
    struct ParticleAllocations
    {
        std::uint64_t count = 0, payloadBytes = 0, allocationBytes = 0;
        unsigned int capacity = 0, stride = 0;
    } particleAllocations;
    std::filesystem::path outputPath;
    struct Sample
    {
        std::string phase;
        std::uint64_t privateBytes, workingSetBytes, peakWorkingSetBytes;
        std::uint64_t localUsage, nonLocalUsage, localBudget;
        ParticleAllocations particles;
        ParticleBufferPool::Statistics pool;
    };
    std::vector<Sample> samples;
    std::string adapterName;
    IngameSkillLoadout measurementLoadout;
    void WriteReport(bool completed)
    {
        std::ofstream report(outputPath);
        if (!report) throw std::runtime_error("메모리 측정 결과 파일 생성 실패");
        report << "{\"ok\":" << (completed ? "true" : "false")
            << ",\"baselineKind\":\"" << (reuseMeasurement ? (dedicatedBuffers ? "dedicated_all_particle_buffers" : "shared_reusable_particle_buffers") : particleMeasurement ? (fullParticlePool ? "full_skill_particle_pool" : "selected_skill_particle_pool") : heroMeasurement ? (fullHeroParts ? "full_hero_parts" : "selected_hero_parts") : (legacyGeometry ? "reconstructed_pre_sharing_geometry_path" : "current_shared_geometry_path"))
            << "\",\"scenario\":\"" << (reuseMeasurement ? "fixed_skills_all_categories_synthetic_particle_replay" : particleMeasurement ? "fresh_process_title_to_ingame_fixed_skills_ogre" : heroMeasurement ? "fresh_process_title_to_ingame_fixed_hero_appearances_ogre" : "fresh_process_title_to_ingame_local_hero_ogre_boss")
            << "\",\"offline\":true,\"heroModels\":{\"loads\":" << heroLoads << ",\"skinnedMeshes\":" << heroSkins
            << ",\"animatedFrames\":" << heroAnimatedFrames << ",\"animationMatrixBytes\":" << heroMatrixBytes
            << "},\"particleReleaseChecked\":" << (particleReleaseChecked ? "true" : "false") << ",\"adapter\":\"" << adapterName
            << "\",\"snapshots\":[";
        for (size_t i = 0; i < samples.size(); ++i)
        {
            if (i) report << ',';
            const auto& s = samples[i];
            report << "{\"phase\":\"" << s.phase << "\",\"privateBytes\":" << s.privateBytes
                << ",\"workingSetBytes\":" << s.workingSetBytes << ",\"peakWorkingSetBytes\":" << s.peakWorkingSetBytes
                << ",\"dxgiLocalUsageBytes\":" << s.localUsage << ",\"dxgiNonLocalUsageBytes\":" << s.nonLocalUsage
                << ",\"dxgiLocalBudgetBytes\":" << s.localBudget
                << ",\"particleCreatedCount\":" << s.particles.count
                << ",\"particleLargeBufferPayloadBytes\":" << s.particles.payloadBytes
                << ",\"particleLargeBufferAllocationBytes\":" << s.particles.allocationBytes
                << ",\"particleCapacity\":" << s.particles.capacity
                << ",\"particleVertexStride\":" << s.particles.stride
                << ",\"poolAllocatedPairs\":" << s.pool.allocatedPairs << ",\"poolLeasedPairs\":" << s.pool.leasedPairs
                << ",\"poolPendingPairs\":" << s.pool.pendingPairs << ",\"poolReuseCount\":" << s.pool.reuseCount
                << ",\"poolAllocationBytes\":" << s.pool.allocationBytes << ",\"poolAllocationFailures\":" << s.pool.allocationFailures << '}';
        }
        report << "]";
        if (particleMeasurement)
        {
            report << ",\"loadout\":{\"jobs\":[";
            for (int player = 0; player < INGAME_PLAYER; ++player) { if (player) report << ','; report << measurementLoadout.jobs[player]; }
            report << "],\"skills\":[";
            for (int player = 0; player < INGAME_PLAYER; ++player)
            {
                if (player) report << ','; report << '[';
                for (int slot = 0; slot < MAX_SKILL; ++slot) { if (slot) report << ','; report << measurementLoadout.skills[player][slot]; }
                report << ']';
            }
            report << "]}";
        }
        report << "}\n";
    }
}
bool ClientMemoryProfileDedicatedParticleBuffers() { return active && (!reuseMeasurement || dedicatedBuffers); }
bool ClientMemoryProfileActive() { return active; }
bool ClientMemoryProfileLegacyGeometry() { return active && legacyGeometry; }
bool ClientMemoryProfileFullHeroParts() { return active && heroMeasurement && fullHeroParts; }
// 기존 geometry/외형 비교 시나리오는 전체 풀을 유지해 이전 근거와 구분한다.
bool ClientMemoryProfileFullParticlePool() { return active && (!particleMeasurement || fullParticlePool); }

void ClientMemoryProfileRecordHeroModel(unsigned int skins, unsigned int animatedFrames, unsigned long long matrixBytes)
{
    if (!active) return;
    ++heroLoads;
    heroSkins += skins;
    heroAnimatedFrames += animatedFrames;
    heroMatrixBytes += matrixBytes;
}

void ClientMemoryProfileRecordParticle(ID3D12Device* device, ID3D12Resource* streamOutput,
    ID3D12Resource* draw, unsigned int capacity, unsigned int stride)
{
    if (!active) return;
    if (!device || !streamOutput || !draw) throw std::runtime_error("파티클 버퍼 계측 실패");
    if (!reuseMeasurement && particleAllocations.count && (capacity != particleAllocations.capacity || stride != particleAllocations.stride))
        throw std::runtime_error("측정 시나리오의 파티클 용량/stride가 일치하지 않음");
    for (auto resource : {streamOutput, draw})
    {
        const auto description = resource->GetDesc();
        const auto allocation = device->GetResourceAllocationInfo(0, 1, &description);
        if (allocation.SizeInBytes == UINT64_MAX) throw std::runtime_error("파티클 할당 정보 조회 실패");
        particleAllocations.payloadBytes += description.Width;
        particleAllocations.allocationBytes += allocation.SizeInBytes;
    }
    ++particleAllocations.count;
    particleAllocations.capacity = capacity;
    particleAllocations.stride = stride;
}

void ClientMemoryProfileSnapshot(ID3D12Device* device, const char* phase, const ParticleBufferPool* pool)
{
    if (!active) return;
    PROCESS_MEMORY_COUNTERS_EX memory{};
    memory.cb = sizeof(memory);
    if (!GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&memory), sizeof(memory)))
        throw std::runtime_error("프로세스 메모리 정보 조회 실패");
    DXGI_QUERY_VIDEO_MEMORY_INFO local{}, nonLocal{};
    if (device)
    {
        Microsoft::WRL::ComPtr<IDXGIFactory4> factory;
        Microsoft::WRL::ComPtr<IDXGIAdapter3> adapter;
        if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory))) ||
            FAILED(factory->EnumAdapterByLuid(device->GetAdapterLuid(), IID_PPV_ARGS(&adapter))) ||
            FAILED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &local)) ||
            FAILED(adapter->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_NON_LOCAL, &nonLocal)))
            throw std::runtime_error("DXGI adapter 메모리 정보 조회 실패");
        DXGI_ADAPTER_DESC2 description{};
        adapter->GetDesc2(&description);
        char name[256]{};
        WideCharToMultiByte(CP_UTF8, 0, description.Description, -1, name, sizeof(name), nullptr, nullptr);
        adapterName = name;
    }
    samples.push_back({phase, memory.PrivateUsage, memory.WorkingSetSize, memory.PeakWorkingSetSize,
        local.CurrentUsage, nonLocal.CurrentUsage, local.Budget, particleAllocations, pool ? pool->GetStatistics() : ParticleBufferPool::Statistics{}});
    WriteReport(false);
}

int RunHeroSelectionAudit(CGameFramework& framework, const wchar_t* reportPath)
{
    active = true;
    outputPath = reportPath;
    NetworkManager::GetInstance()->Initialize("");
    ghAppInstance = GetModuleHandle(nullptr);
    const HWND window = CreateWindowEx(0, L"STATIC", L"Hero selection audit", WS_OVERLAPPEDWINDOW,
        0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, nullptr, nullptr, ghAppInstance, nullptr);
    try
    {
        if (!window || !framework.OnCreate(ghAppInstance, window)) throw std::runtime_error("검증용 클라이언트 초기화 실패");
        const int result = framework.AuditHeroSelection(reportPath);
        framework.OnDestroy();
        DestroyWindow(window);
        return result;
    }
    catch (const std::exception& error)
    {
        OutputDebugStringA(error.what());
        WriteReport(false);
        if (window) DestroyWindow(window);
        return 1;
    }
}

int RunClientMemoryProfile(CGameFramework& framework, const wchar_t* reportPath, bool legacy, bool measureHero, bool fullParts, bool measureParticles, bool fullParticles, bool measureReuse, bool dedicated)
{
    active = true;
    reuseMeasurement = measureReuse; dedicatedBuffers = dedicated;
    legacyGeometry = legacy;
    heroMeasurement = measureHero;
    fullHeroParts = fullParts;
    particleMeasurement = measureParticles;
    fullParticlePool = fullParticles;
    outputPath = reportPath;
    try
    {
        ClientMemoryProfileSnapshot(nullptr, "process_start");
        auto network = NetworkManager::GetInstance();
        network->Initialize("");
        network->SetId(0);
        if (heroMeasurement)
        {
            network->SeedTestIngameAppearances();
            auto appearances = network->m_ArrayInGameClientsCustom;
            appearances[1].Chr_Sex = 1;
            appearances[2].Chr_Torso = 5;
            for (int slot = 0; slot < 3; ++slot) network->StoreIngameAppearance(slot, appearances[slot]);
        }
        network->readySceneInfo->playerJobs = {0, 1, 2, 4};
        for (int i = 0; i < INGAME_PLAYER; ++i)
            for (auto& skill : network->readySceneInfo->selectSkills[i])
                skill = i == 3 ? BOSS_SKILL / 2 : PLAYER_SKILL / 4;
        if (particleMeasurement)
        {
            const int skills[4][4]{{48, 53, 54, 58}, {60, 65, 69, 70}, {72, 79, 81, 82}, {24, 27, 29, 30}};
            for (int player = 0; player < INGAME_PLAYER; ++player)
                for (int slot = 0; slot < MAX_SKILL; ++slot)
                    network->StoreReadySkill(player, slot, skills[player][slot]);
            measurementLoadout = network->GetIngameSkillLoadout();
        }
        SceneManager::GetInstance()->SetOrder(ORDER::PLAYER1);
        const auto instance = GetModuleHandle(nullptr);
        ghAppInstance = instance;
        RECT rectangle{0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT};
        AdjustWindowRect(&rectangle, WS_OVERLAPPEDWINDOW, FALSE);
        const HWND window = CreateWindowEx(0, L"STATIC", L"WOD memory profile", WS_OVERLAPPEDWINDOW,
            0, 0, rectangle.right - rectangle.left, rectangle.bottom - rectangle.top, nullptr, nullptr, instance, nullptr);
        if (!window) throw std::runtime_error("측정용 숨김 창 생성 실패");
        if (!framework.OnCreate(instance, window)) throw std::runtime_error("클라이언트 초기화 실패");
        framework.ProfileMemorySnapshot("title_ready");
        framework.ChangeScene(SCENEKIND::INGAME);
        if (reuseMeasurement)
        {
            framework.ProfileMemorySnapshot("ingame_before_particle_use");
            framework.ProfileParticleReuseFrames();
        }
        framework.ProfileMemorySnapshot("ingame_ready");
        if (particleMeasurement) { framework.ProfileReleaseParticles(); particleReleaseChecked = true; }
        WriteReport(true);
        DestroyWindow(window);
        // 전역 자원은 독립 측정 프로세스 종료 때 회수한다. 종료 경로를 진입 측정에 섞지 않는다.
        return 0;
    }
    catch (const std::exception& error)
    {
        OutputDebugStringA(error.what());
        WriteReport(false);
        return 1;
    }
}

int RunParticleSelectionTestCases(const wchar_t* reportPath);
int RunParticleSelectionTests(const wchar_t* reportPath)
{
    active = true; // 테스트에서는 소켓 연결을 생략한다.
    return RunParticleSelectionTestCases(reportPath);
}
