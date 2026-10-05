#include "stdafx.h"
#include "ObjectConstantArena.h"
#include "d3dx12.h"
#include <stdexcept>
#include <limits>

namespace {
    thread_local std::shared_ptr<ObjectConstantArena> current;
    std::atomic<size_t> livePages{0};
    void CheckArena(HRESULT result) { if (FAILED(result)) throw std::runtime_error("object constant arena GPU 오류"); }
}
struct ObjectConstantArena::Page
{
    Microsoft::WRL::ComPtr<ID3D12Resource> resource;
    BYTE* mapped = nullptr;
    explicit Page(ID3D12Device* device)
    {
        const auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto description = CD3DX12_RESOURCE_DESC::Buffer(PageBytes);
        CheckArena(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &description,
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&resource)));
        const D3D12_RANGE empty{0, 0};
        CheckArena(resource->Map(0, &empty, reinterpret_cast<void**>(&mapped)));
        ++livePages;
    }
    ~Page() { if (mapped) { resource->Unmap(0, nullptr); --livePages; } }
};
ObjectConstantArena::ObjectConstantArena(ID3D12Device* value) : device(value)
{
    if (!value) throw std::invalid_argument("arena device 없음");
    CheckArena(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
    event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event) throw std::runtime_error("arena fence event 생성 실패");
}
ObjectConstantArena::~ObjectConstantArena()
{
    // 정상 경로는 Scene/Framework의 GPU 완료 후 해제다. device removed는 GPU 실행이 중단된 상태다.
    try { WaitForIdle(); } catch (...) { OutputDebugStringA("object arena 종료 fence 실패\n"); }
    if (event) CloseHandle(event);
}
void ObjectConstantArena::EnsureCapacity(size_t draws)
{
    constexpr size_t slotsPerPage = PageBytes / Stride;
    const size_t wanted = draws / slotsPerPage + (draws % slotsPerPage != 0);
    // 두 frame의 새 페이지를 먼저 준비해 실패 시 현재 영역을 유지한다.
    std::vector<std::unique_ptr<Page>> added[FrameCount];
    for (UINT frame = 0; frame < FrameCount; ++frame) {
        frames[frame].pages.reserve(wanted);
        for (size_t page = frames[frame].pages.size(); page < wanted; ++page)
            added[frame].push_back(std::make_unique<Page>(device.Get()));
    }
    for (UINT frame = 0; frame < FrameCount; ++frame)
        for (auto& page : added[frame]) frames[frame].pages.push_back(std::move(page));
}
void ObjectConstantArena::ReserveSlot()
{
    if (slots == (std::numeric_limits<size_t>::max)()) throw std::overflow_error("arena slot overflow");
    EnsureCapacity(slots + 1); ++slots;
}
void ObjectConstantArena::Wait(UINT64 value)
{
    const UINT64 completed = fence->GetCompletedValue();
    if (completed == UINT64_MAX) throw std::runtime_error("arena device removed");
    if (completed >= value) return;
    CheckArena(fence->SetEventOnCompletion(value, event));
    if (WaitForSingleObject(event, INFINITE) != WAIT_OBJECT_0) throw std::runtime_error("arena fence 대기 실패");
}
void ObjectConstantArena::BeginFrame(UINT index)
{
    if (index >= FrameCount) throw std::out_of_range("arena frame 범위 오류");
    if (activeFrame != FrameCount) throw std::logic_error("arena frame 미제출");
    Wait(frames[index].fence);
    frames[index].draws = 0; activeFrame = index;
}
ObjectConstantArena::Location ObjectConstantArena::Write(const void* data, size_t size)
{
    if (activeFrame == FrameCount) throw std::logic_error("arena BeginFrame 필요");
    if (!data || !size || size > Stride) throw std::invalid_argument("arena constant 크기 오류");
    auto& frame = frames[activeFrame];
    if (frame.draws == (std::numeric_limits<size_t>::max)()) throw std::overflow_error("arena draw overflow");
    EnsureCapacity(frame.draws + 1);
    const size_t draw = frame.draws++;
    auto& page = frame.pages[draw / (PageBytes / Stride)];
    const UINT64 offset = (draw % (PageBytes / Stride)) * Stride;
    memset(page->mapped + offset, 0, Stride); memcpy(page->mapped + offset, data, size);
    peakDraws = (std::max)(peakDraws, frame.draws);
    return {page->resource.Get(), offset, page->resource->GetGPUVirtualAddress() + offset};
}
void ObjectConstantArena::SubmitFrame(ID3D12CommandQueue* queue)
{
    if (!queue || activeFrame == FrameCount) throw std::logic_error("arena 제출 상태 오류");
    CheckArena(queue->Signal(fence.Get(), nextFence));
    frames[activeFrame].fence = nextFence++; activeFrame = FrameCount;
}
void ObjectConstantArena::WaitForIdle() { for (const auto& frame : frames) Wait(frame.fence); }
ObjectConstantArena::Statistics ObjectConstantArena::GetStatistics() const
{
    const size_t pages = frames[0].pages.size() + frames[1].pages.size();
    return {slots, pages, pages * PageBytes, peakDraws};
}
size_t ObjectConstantArena::LivePages() { return livePages.load(); }
std::shared_ptr<ObjectConstantArena> ObjectConstantArena::Current() { return current; }
ObjectConstantArena::Scope::Scope(std::shared_ptr<ObjectConstantArena> arena) : previous(std::move(current)) { current = std::move(arena); }
ObjectConstantArena::Scope::~Scope() { current = std::move(previous); }
