#include "stdafx.h"
#include "SharedDdsTexture.h"
#include "Object.h"
#include "DDSTextureLoader12.h"
#include "d3dx12.h"
#include <compare>
#include <cstdint>
#include <filesystem>
#include <map>
#include <atomic>
#include <stdexcept>

namespace
{
    struct Key
    {
        std::uintptr_t device;
        std::wstring path;
        UINT resourceType;
        auto operator<=>(const Key&) const = default;
    };
    std::mutex cacheMutex;
    std::map<Key, std::weak_ptr<SharedDdsTexture>> cache;
    size_t created = 0, hits = 0;
    std::atomic<size_t> live = 0;
    std::atomic<size_t> uploads{0};
    std::atomic<UINT64> defaultBytes{0}, uploadBytes{0};
    void Check(HRESULT hr) { if (FAILED(hr)) throw std::runtime_error("공용 DDS 자원 생성 실패"); }
    std::wstring NormalizedPath(const wchar_t* path)
    {
        if (!path || !*path) throw std::invalid_argument("DDS 경로 없음");
        auto result = std::filesystem::weakly_canonical(std::filesystem::absolute(path)).wstring();
        CharLowerBuffW(result.data(), static_cast<DWORD>(result.size()));
        return result;
    }
}

SharedDdsTexture::SharedDdsTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commands, const wchar_t* path)
    : m_device(device), m_uploadList(commands)
{
    std::unique_ptr<uint8_t[]> data;
    std::vector<D3D12_SUBRESOURCE_DATA> subresources;
    // 기존 모델 DDS 로더와 동일: maxsize=0, flags=NONE, loader=DEFAULT, 전체 mip/array.
    Check(DirectX::LoadDDSTextureFromFileEx(device, path, 0, D3D12_RESOURCE_FLAG_NONE,
        DDS_LOADER_DEFAULT, &m_resource, data, subresources, nullptr, nullptr));
    if (subresources.empty()) throw std::runtime_error("DDS subresource 없음");
    const auto bytes = GetRequiredIntermediateSize(m_resource.Get(), 0, static_cast<UINT>(subresources.size()));
    D3D12_HEAP_PROPERTIES heap{};
    heap.Type = D3D12_HEAP_TYPE_UPLOAD; heap.CreationNodeMask = heap.VisibleNodeMask = 1;
    D3D12_RESOURCE_DESC desc{};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; desc.Width = bytes;
    desc.Height = 1; desc.DepthOrArraySize = desc.MipLevels = 1;
    desc.SampleDesc.Count = 1; desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    Check(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
        D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_upload)));
    if (!UpdateSubresources(commands, m_resource.Get(), m_upload.Get(), 0, 0,
        static_cast<UINT>(subresources.size()), subresources.data()))
        throw std::runtime_error("DDS GPU 복사 기록 실패");
    SynchronizeResourceTransition(commands, m_resource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    const auto resourceDescription = m_resource->GetDesc();
    m_defaultBytes = device->GetResourceAllocationInfo(0, 1, &resourceDescription).SizeInBytes;
    m_uploadBytes = device->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
    defaultBytes += m_defaultBytes; uploadBytes += m_uploadBytes; ++uploads;
    ++live;
}

SharedDdsTexture::~SharedDdsTexture() { ReleaseUpload(); defaultBytes -= m_defaultBytes; --live; }

ID3D12Resource* SharedDdsTexture::Upload() const
{
    std::lock_guard lock(m_uploadMutex);
    return m_upload.Get();
}

void SharedDdsTexture::ReleaseUpload()
{
    std::lock_guard lock(m_uploadMutex);
    if (m_upload) { uploadBytes -= m_uploadBytes; --uploads; }
    m_upload.Reset(); m_uploadList = nullptr;
}

void SharedDdsTexture::CheckUploadList(ID3D12GraphicsCommandList* commands) const
{
    std::lock_guard lock(m_uploadMutex);
    if (m_upload && m_uploadList != commands)
        throw std::logic_error("DDS 업로드 완료 전에 다른 command list에서 공유 요청");
}

std::shared_ptr<SharedDdsTexture> SharedDdsTexture::Load(ID3D12Device* device,
    ID3D12GraphicsCommandList* commands, const wchar_t* path, UINT resourceType)
{
    if (!device || !commands) throw std::invalid_argument("DDS device/command list 없음");
    if (resourceType != RESOURCE_TEXTURE2D && resourceType != RESOURCE_TEXTURE_CUBE &&
        resourceType != RESOURCE_TEXTURE2DARRAY && resourceType != RESOURCE_TEXTURE2D_ARRAY)
        throw std::invalid_argument("공용 DDS texture 용도 오류");
    const auto normalized = NormalizedPath(path);
    const Key key{reinterpret_cast<std::uintptr_t>(device), normalized, resourceType};
    std::lock_guard lock(cacheMutex);
    if (auto found = cache.find(key); found != cache.end())
        if (auto shared = found->second.lock())
        {
            shared->CheckUploadList(commands);
            ++hits; return shared;
        }
    auto shared = std::shared_ptr<SharedDdsTexture>(new SharedDdsTexture(device, commands, normalized.c_str()));
    cache[key] = shared; ++created;
    if (cache.size() > 256)
        std::erase_if(cache, [](const auto& entry) { return entry.second.expired(); });
    return shared;
}

SharedDdsTexture::Statistics SharedDdsTexture::GetStatistics()
{
    std::lock_guard lock(cacheMutex);
    std::erase_if(cache, [](const auto& entry) { return entry.second.expired(); });
    return {created, hits, live.load(), defaultBytes.load(), uploadBytes.load(), uploads.load()};
}
