#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

// 렌더 스레드 전용. 활성 효과는 독립 블록을 임대하고 GPU 완료 후에만 반환 공간을 재사용한다.
class ParticleBufferPool
{
public:
    struct Block
    {
        Microsoft::WRL::ComPtr<ID3D12Resource> streamOutput, draw;
        UINT capacity = 0, stride = 0;
        UINT64 lastUseFence = 0, allocationBytes = 0;
        bool leased = false, initialized = false;
    };
    struct Statistics
    {
        size_t allocatedPairs = 0, leasedPairs = 0, pendingPairs = 0, peakLeasedPairs = 0;
        UINT64 allocationBytes = 0, reuseCount = 0, allocationFailures = 0;
    };

    explicit ParticleBufferPool(ID3D12Device* device);
    ~ParticleBufferPool();
    ParticleBufferPool(const ParticleBufferPool&) = delete;
    ParticleBufferPool& operator=(const ParticleBufferPool&) = delete;
    Block* Acquire(UINT capacity, UINT stride);
    void Return(Block* block);
    void BeginFrame();
    void Touch(Block* block);
    void SubmitFrame(ID3D12CommandQueue* queue);
    void WaitForIdle();
    Statistics GetStatistics() const;
    static size_t LivePairs();

private:
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
    std::vector<std::unique_ptr<Block>> m_blocks;
    UINT64 m_recordingFence = 0, m_lastSubmitted = 0;
    size_t m_peakLeased = 0;
    UINT64 m_reuseCount = 0, m_failures = 0;
};
