#include "stdafx.h"
#include "ObjectConstantArena.h"
#include "Object.h"
#include "d3dx12.h"
#include <future>
#include <filesystem>
#include <stdexcept>

namespace {
    using Microsoft::WRL::ComPtr;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void Check(HRESULT value) { Require(SUCCEEDED(value), "arena test GPU API 실패"); }
    template<class E, class F> void Reject(F action) {
        bool rejected = false; try { action(); } catch (const E&) { rejected = true; }
        Require(rejected, "arena 실패 경로 미거부");
    }
}
int RunObjectConstantArenaTests(const wchar_t* reportPath)
{
    std::ofstream report{std::filesystem::path(reportPath)};
    try {
        ComPtr<ID3D12Debug> debug; Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))); debug->EnableDebugLayer();
        ComPtr<IDXGIFactory4> factory; Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
        ComPtr<IDXGIAdapter> adapter; Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
        ComPtr<ID3D12Device> device; Check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
        ComPtr<ID3D12InfoQueue> diagnostics; Check(device.As(&diagnostics));
        ComPtr<ID3D12CommandQueue> queue; const D3D12_COMMAND_QUEUE_DESC queueDesc{};
        Check(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&queue)));
        ComPtr<ID3D12CommandAllocator> allocator; Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
        ComPtr<ID3D12GraphicsCommandList> list; Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        Reject<std::invalid_argument>([] { ObjectConstantArena invalid(nullptr); });
        const size_t baseline = ObjectConstantArena::LivePages();
        auto arena = std::make_shared<ObjectConstantArena>(device.Get());
        Reject<std::logic_error>([&] { UINT v = 0; arena->Write(&v, sizeof(v)); });
        Reject<std::out_of_range>([&] { arena->BeginFrame(2); });
        Reject<std::logic_error>([&] { arena->SubmitFrame(queue.Get()); });
        for (int slot = 0; slot < 300; ++slot) arena->ReserveSlot();
        Require(arena->GetStatistics().pages == 4 && arena->GetStatistics().bytes == 262144, "300 slot 2-frame 페이지 비용 오류");
        arena->BeginFrame(0);
        Reject<std::logic_error>([&] { arena->BeginFrame(1); });
        Reject<std::invalid_argument>([&] { arena->Write(nullptr, 1); });
        UINT first = 0x12345678, second = 0xabcdef09;
        Reject<std::invalid_argument>([&] { arena->Write(&first, 257); });
        const auto a = arena->Write(&first, sizeof(first)), b = arena->Write(&second, sizeof(second));
        Require(a.resource == b.resource && a.offset != b.offset && a.address % 256 == 0 && b.address % 256 == 0,
            "동일 frame draw 값 독립성/정렬 오류");
        // 예상 draw 수를 넘으면 새 페이지를 추가하고 기존 GPU 주소는 유지한다.
        for (UINT draw = 2; draw < 600; ++draw) arena->Write(&draw, sizeof(draw));
        Require(arena->GetStatistics().pages == 6 && arena->GetStatistics().peakDraws == 600, "arena 확장 실패");
        const auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(512);
        ComPtr<ID3D12Resource> readback;
        Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
        list->CopyBufferRegion(readback.Get(), 0, a.resource, a.offset, 256);
        list->CopyBufferRegion(readback.Get(), 256, b.resource, b.offset, 256);
        Check(list->Close()); ID3D12CommandList* lists[]{list.Get()}; queue->ExecuteCommandLists(1, lists);
        arena->SubmitFrame(queue.Get()); arena->WaitForIdle();
        void* mapped; const D3D12_RANGE range{0, 512}; Check(readback->Map(0, &range, &mapped));
        Require(*reinterpret_cast<UINT*>(mapped) == first && *reinterpret_cast<UINT*>(static_cast<BYTE*>(mapped) + 256) == second,
            "확장 이후 실제 GPU readback 값 변경");
        const D3D12_RANGE empty{0,0}; readback->Unmap(0, &empty);
        arena->BeginFrame(1);
        const auto c = arena->Write(&second, sizeof(second));
        Require(c.resource != a.resource, "frame 사이 물리 영역 공유 오류");
        // GPU 제출을 gate fence 뒤에 대기시켜 frame 재사용이 완료까지 막히는지 검사한다.
        ComPtr<ID3D12Fence> gate; Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&gate)));
        Check(queue->Wait(gate.Get(), 1)); arena->SubmitFrame(queue.Get());
        auto waiter = std::async(std::launch::async, [&] { arena->BeginFrame(1); });
        const bool blocked = waiter.wait_for(std::chrono::milliseconds(100)) == std::future_status::timeout;
        Check(gate->Signal(1)); waiter.get(); Require(blocked, "GPU 사용 중 frame 영역 재사용");
        const auto d = arena->Write(&first, sizeof(first));
        Require(d.resource == c.resource && d.offset == 0, "완료 frame 영역 재사용 실패");
        arena->SubmitFrame(queue.Get()); arena->WaitForIdle(); arena->WaitForIdle();
        {
            ObjectConstantArena::Scope scope(arena); Require(ObjectConstantArena::Current() == arena, "scope owner 오류");
            { ObjectConstantArena::Scope nested(nullptr); Require(!ObjectConstantArena::Current(), "nested scope 오류"); }
            Require(ObjectConstantArena::Current() == arena, "scope 복원 실패");
            CGameObject object;
            object.CreateShaderVariables(device.Get(), nullptr);
            Require(object.m_pd3dcbVecObject[0] == nullptr && object.m_objectConstantOwners[0] == arena, "객체 arena 연결 실패");
            object.ReleaseShaderVariables(); object.ReleaseShaderVariables();
            Require(object.m_objectConstantOwners.empty() && object.m_pcbMappedVecObjects.empty(), "객체 반복 해제 실패");
        }
        Require(!ObjectConstantArena::Current(), "scope owner 잔류");
        {
            CMaterial material(0); material.CreateShaderVariables(device.Get(), nullptr);
            const auto resource = material.m_pd3dcbMaterial;
            material.CreateShaderVariables(device.Get(), nullptr);
            Require(material.m_pd3dcbMaterial == resource, "공용 재질 CB 중복 생성");
        }
        const auto stats = arena->GetStatistics(); arena.reset();
        Require(ObjectConstantArena::LivePages() == baseline, "arena 종료 페이지 잔류");
        UINT64 warnings = 0;
        for (UINT64 i = 0; i < diagnostics->GetNumStoredMessages(); ++i) {
            SIZE_T size = 0; Check(diagnostics->GetMessage(i, nullptr, &size)); std::vector<char> bytes(size);
            auto message = reinterpret_cast<D3D12_MESSAGE*>(bytes.data()); Check(diagnostics->GetMessage(i, message, &size));
            if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) throw std::runtime_error(message->pDescription);
            if (message->Severity == D3D12_MESSAGE_SEVERITY_WARNING) ++warnings;
        }
        report << "{\"ok\":true,\"checks\":10,\"slots\":" << stats.slots << ",\"pages\":" << stats.pages
            << ",\"bytes\":" << stats.bytes << ",\"peakDraws\":" << stats.peakDraws
            << ",\"liveAfterExit\":0,\"gpuErrors\":0,\"gpuWarnings\":" << warnings << "}\n";
        return 0;
    } catch (const std::exception& error) {
        report << "{\"ok\":false}";
        std::ofstream(std::filesystem::path(std::wstring(reportPath) + L".error.txt")) << error.what(); return 1;
    }
}
