#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

// 객체 값은 공유하지 않는다. 프레임마다 완료 fence 이후 256-byte draw 영역을 재사용한다.
class ObjectConstantArena
{
public:
    static constexpr UINT FrameCount = 2, PageBytes = 65536, Stride = 256;
    struct Location { ID3D12Resource* resource; UINT64 offset; D3D12_GPU_VIRTUAL_ADDRESS address; };
    struct Statistics { size_t slots, pages, bytes, peakDraws; };
    explicit ObjectConstantArena(ID3D12Device* device);
    ~ObjectConstantArena();
    ObjectConstantArena(const ObjectConstantArena&) = delete;
    ObjectConstantArena& operator=(const ObjectConstantArena&) = delete;
    void ReserveSlot();
    void BeginFrame(UINT index);
    Location Write(const void* data, size_t size);
    void SubmitFrame(ID3D12CommandQueue* queue);
    void WaitForIdle();
    Statistics GetStatistics() const;
    static size_t LivePages();
    static std::shared_ptr<ObjectConstantArena> Current();
    class Scope
    {
    public:
        explicit Scope(std::shared_ptr<ObjectConstantArena> arena);
        ~Scope();
        Scope(const Scope&) = delete;
        Scope& operator=(const Scope&) = delete;
    private:
        std::shared_ptr<ObjectConstantArena> previous;
    };
private:
    struct Page;
    struct Frame { std::vector<std::unique_ptr<Page>> pages; size_t draws = 0; UINT64 fence = 0; };
    void EnsureCapacity(size_t draws);
    void Wait(UINT64 value);
    Microsoft::WRL::ComPtr<ID3D12Device> device;
    Microsoft::WRL::ComPtr<ID3D12Fence> fence;
    HANDLE event = nullptr;
    Frame frames[FrameCount];
    UINT activeFrame = FrameCount;
    UINT64 nextFence = 1;
    size_t slots = 0, peakDraws = 0;
};
