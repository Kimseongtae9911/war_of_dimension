#include "stdafx.h"
#include "ParticleBufferPool.h"
#include "ClientMemoryProfile.h"
#include <atomic>
#include <stdexcept>

namespace
{
    std::atomic<size_t> livePairs = 0;
    void Check(HRESULT result)
    {
        if (FAILED(result)) throw std::runtime_error("파티클 풀 GPU 동기화 실패");
    }
}

ParticleBufferPool::ParticleBufferPool(ID3D12Device* device) : m_device(device)
{
    if (!device) throw std::invalid_argument("파티클 풀 device 없음");
    Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
}

ParticleBufferPool::~ParticleBufferPool()
{
    // 정상 종료/장면 전환은 먼저 queue를 기다린다. 소유자 소멸 시에도 마지막 제출을 확인한다.
    try { WaitForIdle(); }
    catch (...) { OutputDebugStringA("ParticleBufferPool: device/fence failure during shutdown\n"); }
    livePairs.fetch_sub(m_blocks.size());
}

ParticleBufferPool::Block* ParticleBufferPool::Acquire(UINT capacity, UINT stride)
{
    if (!capacity || !stride || UINT64(capacity) * stride > UINT_MAX)
        throw std::invalid_argument("파티클 버퍼 용량 범위 초과");
    const auto completed = m_fence->GetCompletedValue();
    Block* best = nullptr;
    for (auto& block : m_blocks)
        if (!block->leased && block->lastUseFence <= completed && block->stride == stride &&
            block->capacity >= capacity && (!best || block->capacity < best->capacity)) best = block.get();
    if (best)
    {
        best->leased = true;
        ++m_reuseCount;
    }
    else
    {
        auto block = std::make_unique<Block>();
        block->capacity = capacity; block->stride = stride;
        D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        heap.CreationNodeMask = heap.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC desc{};
        desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        desc.Width = UINT64(capacity) * stride;
        desc.Height = 1; desc.DepthOrArraySize = 1; desc.MipLevels = 1;
        desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        // D3D12 buffer는 COMMON으로 생성 후 첫 GPU 사용 시 명시적으로 상태를 정한다.
        auto result = m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&block->streamOutput));
        if (SUCCEEDED(result)) result = m_device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
            D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&block->draw));
        if (FAILED(result))
        {
            ++m_failures;
            OutputDebugStringA("ParticleBufferPool: allocation failed; retaining existing effect state\n");
            return nullptr; // 부분 할당은 ComPtr가 회수한다.
        }
        block->allocationBytes = 2 * m_device->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
        block->leased = true;
        best = block.get();
        m_blocks.push_back(std::move(block));
        ++livePairs;
        ClientMemoryProfileRecordParticle(m_device.Get(), best->streamOutput.Get(), best->draw.Get(), capacity, stride);
    }
    m_peakLeased = (std::max)(m_peakLeased, GetStatistics().leasedPairs);
    return best;
}

void ParticleBufferPool::Return(Block* block)
{
    if (!block) return;
    if (!block->leased) throw std::logic_error("파티클 버퍼 중복 반환");
    block->leased = false;
}

void ParticleBufferPool::BeginFrame()
{
    if (m_recordingFence) throw std::logic_error("파티클 프레임 중복 시작");
    m_recordingFence = m_lastSubmitted + 1;
}

void ParticleBufferPool::Touch(Block* block)
{
    if (!m_recordingFence || !block || !block->leased) throw std::logic_error("파티클 버퍼 사용 범위 오류");
    block->lastUseFence = m_recordingFence;
}

void ParticleBufferPool::SubmitFrame(ID3D12CommandQueue* queue)
{
    if (!m_recordingFence) return;
    Check(queue->Signal(m_fence.Get(), m_recordingFence));
    m_lastSubmitted = m_recordingFence;
    m_recordingFence = 0;
}

void ParticleBufferPool::WaitForIdle()
{
    if (m_fence->GetCompletedValue() >= m_lastSubmitted) return;
    HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (!event) throw std::runtime_error("파티클 fence event 생성 실패");
    const auto result = m_fence->SetEventOnCompletion(m_lastSubmitted, event);
    const auto wait = SUCCEEDED(result) ? WaitForSingleObject(event, INFINITE) : WAIT_FAILED;
    CloseHandle(event);
    Check(result);
    if (wait != WAIT_OBJECT_0) throw std::runtime_error("파티클 fence 대기 실패");
}

ParticleBufferPool::Statistics ParticleBufferPool::GetStatistics() const
{
    Statistics stats{};
    stats.allocatedPairs = m_blocks.size(); stats.peakLeasedPairs = m_peakLeased;
    stats.reuseCount = m_reuseCount; stats.allocationFailures = m_failures;
    const auto completed = m_fence->GetCompletedValue();
    for (auto& block : m_blocks)
    {
        stats.allocationBytes += block->allocationBytes;
        if (block->leased) ++stats.leasedPairs;
        else if (block->lastUseFence > completed) ++stats.pendingPairs;
    }
    return stats;
}

size_t ParticleBufferPool::LivePairs() { return livePairs.load(); }
