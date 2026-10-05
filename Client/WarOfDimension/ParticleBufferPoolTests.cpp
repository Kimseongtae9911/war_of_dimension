#include "stdafx.h"
#include "Mesh.h"
#include <filesystem>
#include <stdexcept>
#include <cstring>

namespace
{
    using Microsoft::WRL::ComPtr;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void Check(HRESULT result) { Require(SUCCEEDED(result), "GPU test API failure"); }
    struct Gpu
    {
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        ComPtr<ID3D12InfoQueue> diagnostics;
        ComPtr<ID3D12RootSignature> root;
        ComPtr<ID3D12PipelineState> pipeline;
        Gpu()
        {
            ComPtr<ID3D12Debug> debug;
            Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))); debug->EnableDebugLayer();
            ComPtr<IDXGIFactory4> factory; ComPtr<IDXGIAdapter> adapter;
            Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
            Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
            Check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
            Check(device.As(&diagnostics));
            D3D12_COMMAND_QUEUE_DESC q{}; Check(device->CreateCommandQueue(&q, IID_PPV_ARGS(&queue)));
            Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
            Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
            D3D12_ROOT_SIGNATURE_DESC rs{};
            rs.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
            ComPtr<ID3DBlob> signature, error;
            Check(D3D12SerializeRootSignature(&rs, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
            Check(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&root)));
            const char* source = R"(
struct V { float3 pos : POSITION; float3 velocity : VELOCITY; float life : LIFETIME; uint type : PARTICLETYPE; };
V VS(V v) { return v; }
[maxvertexcount(12)] void GS(point V input[1], inout PointStream<V> output) {
    V v = input[0];
    if (v.type < 10) { uint seed = v.type; v.type += 10;
        for (uint i = 0; i < 12; ++i) { v.pos = float3(seed, i, 3); v.life = 7; output.Append(v); }
    } else { v.life += 1; output.Append(v); }
})";
            ComPtr<ID3DBlob> vs, gs;
            Check(D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, "VS", "vs_5_1", 0, 0, &vs, &error));
            Check(D3DCompile(source, strlen(source), nullptr, nullptr, nullptr, "GS", "gs_5_1", 0, 0, &gs, &error));
            D3D12_INPUT_ELEMENT_DESC input[] = {
                {"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"VELOCITY",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"LIFETIME",0,DXGI_FORMAT_R32_FLOAT,0,24,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
                {"PARTICLETYPE",0,DXGI_FORMAT_R32_UINT,0,28,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};
            D3D12_SO_DECLARATION_ENTRY so[] = {{0,"POSITION",0,0,3,0},{0,"VELOCITY",0,0,3,0},{0,"LIFETIME",0,0,1,0},{0,"PARTICLETYPE",0,0,1,0}};
            UINT stride = 32;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
            desc.pRootSignature = root.Get(); desc.VS = {vs->GetBufferPointer(), vs->GetBufferSize()};
            desc.GS = {gs->GetBufferPointer(), gs->GetBufferSize()};
            desc.InputLayout = {input, 4}; desc.StreamOutput = {so, 4, &stride, 1, D3D12_SO_NO_RASTERIZED_STREAM};
            desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID; desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            desc.SampleMask = UINT_MAX; desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; desc.SampleDesc.Count = 1;
            Check(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(&pipeline)));
        }
        void Execute(ParticleBufferPool& pool)
        {
            Check(list->Close()); ID3D12CommandList* lists[]{list.Get()}; queue->ExecuteCommandLists(1, lists);
            pool.SubmitFrame(queue.Get()); pool.WaitForIdle();
        }
        void Reset()
        {
            Check(allocator->Reset()); Check(list->Reset(allocator.Get(), pipeline.Get()));
            list->SetGraphicsRootSignature(root.Get());
        }
        void Simulate(CParticleMesh& mesh)
        {
            list->SetPipelineState(pipeline.Get()); list->SetGraphicsRootSignature(root.Get());
            Require(mesh.PrepareBuffers(list.Get()), "mesh buffer allocation");
            mesh.PreRender(list.Get(), 0); mesh.Render(list.Get(), 0);
            list->SOSetTargets(0, 0, nullptr); mesh.PreRender(list.Get(), 1);
        }
        std::vector<CParticleVertex> Read(CParticleMesh& mesh, ParticleBufferPool& pool)
        {
            Reset(); pool.BeginFrame();
            const auto bytes = UINT64(mesh.LiveParticleCount()) * sizeof(CParticleVertex);
            ComPtr<ID3D12Resource> readback;
            D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK; heap.CreationNodeMask = heap.VisibleNodeMask = 1;
            auto desc = mesh.m_pd3dDrawBuffer->GetDesc(); desc.Width = bytes;
            Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
            SynchronizeResourceTransition(list.Get(), mesh.m_pd3dDrawBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_SOURCE);
            list->CopyBufferRegion(readback.Get(), 0, mesh.m_pd3dDrawBuffer, 0, bytes);
            SynchronizeResourceTransition(list.Get(), mesh.m_pd3dDrawBuffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            Execute(pool);
            void* ptr = nullptr; D3D12_RANGE range{0, static_cast<SIZE_T>(bytes)}; Check(readback->Map(0, &range, &ptr));
            std::vector<CParticleVertex> result(mesh.LiveParticleCount()); memcpy(result.data(), ptr, static_cast<size_t>(bytes));
            D3D12_RANGE noWrite{0,0}; readback->Unmap(0, &noWrite); return result;
        }
        UINT64 CheckDiagnostics()
        {
            UINT64 warnings = 0;
            for (UINT64 i = 0; i < diagnostics->GetNumStoredMessages(); ++i)
            {
                SIZE_T size = 0; Check(diagnostics->GetMessage(i, nullptr, &size)); std::vector<char> bytes(size);
                auto message = reinterpret_cast<D3D12_MESSAGE*>(bytes.data()); Check(diagnostics->GetMessage(i, message, &size));
                if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) throw std::runtime_error(message->pDescription);
                if (message->Severity == D3D12_MESSAGE_SEVERITY_WARNING) ++warnings;
            }
            return warnings;
        }
    };
}

int RunParticleBufferPoolTests(const wchar_t* reportPath)
{
    std::ofstream report{std::filesystem::path(reportPath)};
    try
    {
        Require(bool(report), "report open failed");
        const auto initialLive = ParticleBufferPool::LivePairs();
        Gpu gpu;
        {
            auto pool = std::make_shared<ParticleBufferPool>(gpu.device.Get());
            auto a = pool->Acquire(16, 32); auto b = pool->Acquire(16, 32);
            Require(a && b && a != b, "concurrent leases overlap");
            pool->Return(a); auto reused = pool->Acquire(8, 32);
            Require(reused == a && pool->GetStatistics().allocatedPairs == 2, "cross effect reuse failed");
            pool->Return(reused);
            bool rejected = false;
            try { pool->Acquire(UINT_MAX, 32); } catch (const std::invalid_argument&) { rejected = true; }
            Require(rejected, "invalid capacity accepted");
            // 실제 queue를 gate로 멈추고 미완료 fence의 블록을 재사용하지 않는지 확인한다.
            ComPtr<ID3D12Fence> gate; Check(gpu.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate)));
            pool->BeginFrame(); pool->Touch(a = pool->Acquire(16,32)); pool->Return(a);
            Check(gpu.queue->Wait(gate.Get(), 1)); pool->SubmitFrame(gpu.queue.Get());
            auto pending = pool->GetStatistics(); auto c = pool->Acquire(16,32);
            const bool isolated = c && c != a && c != b;
            Check(gate->Signal(1)); pool->WaitForIdle();
            Require(isolated && pending.pendingPairs == 1, "pending GPU block reused");
            pool->Return(c); pool->Return(b);
            Require(pool->GetStatistics().allocatedPairs == 3, "pool did not expand");
        }
        Require(ParticleBufferPool::LivePairs() == initialLive, "expanded pool not released");
        UINT64 peakPairs = 0, growthCapacity = 0, reuseCount = 0;
        // 재진입을 포함해 다른 종류의 실제 GPU 출력과 용량 확장을 검증한다.
        for (int cycle = 0; cycle < 2; ++cycle)
        {
            auto pool = std::make_shared<ParticleBufferPool>(gpu.device.Get());
            if (cycle) gpu.Reset();
            {
                CParticleMesh skill(gpu.device.Get(), gpu.list.Get(), {}, {}, 0, {}, {}, {}, 4, 1, pool);
                CParticleMesh environment(gpu.device.Get(), gpu.list.Get(), {}, {}, 0, {}, {}, {}, 16, 2, pool);
                Require(pool->GetStatistics().allocatedPairs == 0, "inactive meshes allocated large buffers");
                pool->BeginFrame(); gpu.Simulate(skill); gpu.Simulate(environment); gpu.Execute(*pool);
                skill.ParticlePostRender(0); environment.ParticlePostRender(0);
                Require(skill.OverflowCount() == 1 && skill.LiveParticleCount() == 4 && environment.LiveParticleCount() == 12, "stream output overflow statistics incorrect");
                auto first = gpu.Read(skill, *pool); auto second = gpu.Read(environment, *pool);
                Require(first[0].m_xmf3Position.x == 1 && second[0].m_xmf3Position.x == 2, "effect state mixed");
                gpu.Reset(); pool->BeginFrame(); gpu.Simulate(skill); gpu.Simulate(environment); gpu.Execute(*pool);
                skill.ParticlePostRender(0); environment.ParticlePostRender(0);
                auto grown = gpu.Read(skill, *pool);
                Require(skill.m_nMaxParticles >= 12 && grown.size() == first.size(), "effect growth lost live count");
                for (size_t i = 0; i < grown.size(); ++i)
                    Require(grown[i].m_xmf3Position.x == first[i].m_xmf3Position.x && grown[i].m_xmf3Position.y == first[i].m_xmf3Position.y && grown[i].m_fLifetime == first[i].m_fLifetime + 1, "grown buffer state not preserved");
                growthCapacity = skill.m_nMaxParticles;
                const auto oldEnvironmentBuffer = environment.m_pd3dDrawBuffer;
                environment.ReleaseInactiveBuffers();
                // 새 효과는 emitter seed/counter를 초기화하고 이전 효과의 입자를 이어받지 않는다.
                gpu.Reset(); CParticleMesh coin(gpu.device.Get(), gpu.list.Get(), {}, {}, 0, {}, {}, {}, 16, 3, pool);
                pool->BeginFrame(); gpu.Simulate(coin); gpu.Execute(*pool); coin.ParticlePostRender(0);
                auto coins = gpu.Read(coin, *pool);
                Require(coins.size() == 12 && coins[0].m_xmf3Position.x == 3 && coins[0].m_fLifetime == 7, "reused effect seed/counter not reset");
                Require(coin.m_pd3dStreamOutputBuffer == oldEnvironmentBuffer, "different effect did not reuse GPU pair");
                const auto stats = pool->GetStatistics(); peakPairs = stats.allocatedPairs; reuseCount = stats.reuseCount;
                Require(reuseCount >= 1 && stats.allocatedPairs == 3, "unexpected resource growth");
                skill.ReleaseInactiveBuffers(); skill.ReleaseInactiveBuffers(); coin.ReleaseInactiveBuffers();
                Require(pool->GetStatistics().leasedPairs == 0, "inactive leases remain");
                skill.ReleaseUploadBuffers(); environment.ReleaseUploadBuffers(); coin.ReleaseUploadBuffers();
            }
            pool.reset(); Require(ParticleBufferPool::LivePairs() == initialLive, "match exit leak");
        }
        auto warnings = gpu.CheckDiagnostics();
        report << "{\"ok\":true,\"adapter\":\"WARP\",\"cycles\":2,\"pendingFenceIsolation\":true,\"crossEffectStateIsolation\":true,\"growthPreservedState\":true,\"reusedSeedReset\":true,\"peakPairs\":" << peakPairs
            << ",\"grownCapacity\":" << growthCapacity << ",\"reuseCount\":" << reuseCount << ",\"livePairsAfterExit\":" << ParticleBufferPool::LivePairs() << ",\"gpuErrors\":0,\"gpuWarnings\":" << warnings << "}\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        OutputDebugStringA(error.what());
        // JSON 문자열의 세부 진단은 별도 텍스트 파일로 남긴다.
        std::ofstream detail{std::filesystem::path(reportPath).wstring() + L".error.txt"}; detail << error.what();
        report << "{\"ok\":false}\n"; return 1;
    }
}
