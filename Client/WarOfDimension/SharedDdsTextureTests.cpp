#include "stdafx.h"
#include "Object.h"
#include "Scene.h"
#include "SharedDdsTexture.h"
#include "DDSTextureLoader12.h"
#include <filesystem>
#include <stdexcept>
#include <set>
#include <cstring>

namespace
{
    using Microsoft::WRL::ComPtr;
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    void Check(HRESULT hr) { Require(SUCCEEDED(hr), "DDS test GPU API 실패"); }
    template<class Exception, class F> void Reject(F action, const char* message)
    {
        bool rejected = false;
        try { action(); } catch (const Exception&) { rejected = true; }
        Require(rejected, message);
    }
    struct Gpu
    {
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12GraphicsCommandList> list;
        ComPtr<ID3D12InfoQueue> diagnostics;
        std::map<UINT, UINT64> warningIds;
        explicit Gpu(bool hardware = false)
        {
            ComPtr<ID3D12Debug> debug;
            Check(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))); debug->EnableDebugLayer();
            ComPtr<IDXGIFactory4> factory; ComPtr<IDXGIAdapter> adapter;
            Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
            if (hardware)
            {
                // D3D12는 같은 adapter의 device를 재사용하므로 격리 검사는 다른 adapter를 쓴다.
                ComPtr<IDXGIAdapter1> candidate;
                for (UINT index = 0; SUCCEEDED(factory->EnumAdapters1(index, &candidate)); ++index)
                {
                    DXGI_ADAPTER_DESC1 desc{}; Check(candidate->GetDesc1(&desc));
                    if (!(desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)) { Check(candidate.As(&adapter)); break; }
                    candidate.Reset();
                }
                Require(adapter != nullptr, "device 격리 검사용 hardware adapter 없음");
            }
            else Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
            Check(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
            Check(device.As(&diagnostics));
            D3D12_COMMAND_QUEUE_DESC desc{};
            Check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
            Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
            Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
        }
        void Execute()
        {
            Check(list->Close()); ID3D12CommandList* lists[]{list.Get()}; queue->ExecuteCommandLists(1, lists);
            ComPtr<ID3D12Fence> fence; Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
            HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr); Require(event != nullptr, "fence event 실패");
            Check(queue->Signal(fence.Get(), 1)); Check(fence->SetEventOnCompletion(1, event));
            auto waited = WaitForSingleObject(event, 30000); CloseHandle(event);
            Require(waited == WAIT_OBJECT_0, "DDS GPU fence 대기 실패");
        }
        void Reset() { Check(allocator->Reset()); Check(list->Reset(allocator.Get(), nullptr)); }
        UINT64 Diagnostics()
        {
            UINT64 warnings = 0;
            for (UINT64 i = 0; i < diagnostics->GetNumStoredMessages(); ++i)
            {
                SIZE_T size = 0; Check(diagnostics->GetMessage(i, nullptr, &size)); std::vector<char> bytes(size);
                auto message = reinterpret_cast<D3D12_MESSAGE*>(bytes.data()); Check(diagnostics->GetMessage(i, message, &size));
                if (message->Severity <= D3D12_MESSAGE_SEVERITY_ERROR) throw std::runtime_error(message->pDescription);
                if (message->Severity == D3D12_MESSAGE_SEVERITY_WARNING) { ++warnings; ++warningIds[message->ID]; }
            }
            return warnings;
        }
    };

    void VerifyPixels(Gpu& gpu, SharedDdsTexture& texture, const wchar_t* path)
    {
        // 실제 GPU 복사 결과를 DDS의 첫 mip 전체 행과 비교한다.
        ComPtr<ID3D12Resource> reference;
        std::unique_ptr<uint8_t[]> data; std::vector<D3D12_SUBRESOURCE_DATA> subs;
        Check(DirectX::LoadDDSTextureFromFileEx(gpu.device.Get(), path, 0, D3D12_RESOURCE_FLAG_NONE,
            DDS_LOADER_DEFAULT, &reference, data, subs, nullptr, nullptr));
        auto desc = texture.Resource()->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{}; UINT rows = 0; UINT64 rowBytes = 0, total = 0;
        gpu.device->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &rowBytes, &total);
        D3D12_HEAP_PROPERTIES heap{}; heap.Type = D3D12_HEAP_TYPE_READBACK; heap.CreationNodeMask = heap.VisibleNodeMask = 1;
        D3D12_RESOURCE_DESC buffer{}; buffer.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; buffer.Width = total;
        buffer.Height = 1; buffer.DepthOrArraySize = buffer.MipLevels = 1; buffer.SampleDesc.Count = 1; buffer.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        ComPtr<ID3D12Resource> readback;
        Check(gpu.device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &buffer, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
        gpu.Reset();
        SynchronizeResourceTransition(gpu.list.Get(), texture.Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_SOURCE);
        D3D12_TEXTURE_COPY_LOCATION src{}; src.pResource = texture.Resource(); src.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION dst{}; dst.pResource = readback.Get(); dst.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT; dst.PlacedFootprint = footprint;
        gpu.list->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        SynchronizeResourceTransition(gpu.list.Get(), texture.Resource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_GENERIC_READ);
        gpu.Execute();
        void* mapped = nullptr; D3D12_RANGE range{0, static_cast<SIZE_T>(total)}; Check(readback->Map(0, &range, &mapped));
        bool equal = true;
        for (UINT row = 0; row < rows; ++row)
            equal &= !memcmp(static_cast<uint8_t*>(mapped) + footprint.Offset + size_t(row) * footprint.Footprint.RowPitch,
                static_cast<const uint8_t*>(subs[0].pData) + size_t(row) * subs[0].RowPitch, static_cast<size_t>(rowBytes));
        D3D12_RANGE noWrite{0,0}; readback->Unmap(0, &noWrite);
        Require(equal, "GPU DDS pixel 내용 불일치");
    }

    struct Model
    {
        CLoadedModelInfo* info = nullptr;
        ~Model()
        {
            if (!info) return;
            if (info->m_pAnimationSets) info->m_pAnimationSets->Release();
            info->m_pModelRootObject->Release(); delete info;
        }
    };
    void Textures(CGameObject* root, std::vector<CTexture*>& textures)
    {
        for (auto frame = root; frame; frame = frame->m_pSibling)
        {
            for (int m = 0; m < frame->m_nMaterials; ++m)
                for (int t = 0; t < frame->m_ppMaterials[m]->m_nTextures; ++t)
                    if (auto texture = frame->m_ppMaterials[m]->m_ppTextures[t]) textures.push_back(texture);
            if (frame->m_pChild) Textures(frame->m_pChild, textures);
        }
    }
}

int RunDdsSharingTests(const wchar_t* reportPath)
{
    std::ofstream report{std::filesystem::path(reportPath)};
    try
    {
        Require(bool(report), "DDS 검사 report 열기 실패");
        Require(SharedDdsTexture::GetStatistics().live == 0, "초기 공용 DDS 잔류");
        Gpu gpu;
        const auto path = L"Model/Textures/AlbedoPBR.dds";
        {
            auto a = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), path, RESOURCE_TEXTURE2D);
            auto b = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), L"Model/Textures/../Textures/ALBEDOPBR.DDS", RESOURCE_TEXTURE2D);
            Require(a == b && a->Upload() != nullptr, "정규 경로 공유 실패");
            auto otherPath = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), L"Model/Textures/MSPBR.dds", RESOURCE_TEXTURE2D);
            Require(otherPath != a, "다른 DDS 경로 충돌");
            auto usage = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), path, RESOURCE_TEXTURE2DARRAY);
            Require(usage != a, "다른 SRV 용도 충돌");
            ComPtr<ID3D12CommandAllocator> allocator; ComPtr<ID3D12GraphicsCommandList> anotherList;
            Check(gpu.device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
            Check(gpu.device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&anotherList)));
            Reject<std::logic_error>([&] { SharedDdsTexture::Load(gpu.device.Get(), anotherList.Get(), path, RESOURCE_TEXTURE2D); }, "pending 다른 list 허용");
            Check(anotherList->Close());
            auto beforeFailure = SharedDdsTexture::GetStatistics();
            Reject<std::runtime_error>([&] { SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), L"Model/Textures/not-existing-dds-test.dds", RESOURCE_TEXTURE2D); }, "없는 DDS 허용");
            Reject<std::runtime_error>([&] { SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), L"Model/Monsters/Normal/ChestMonsterPBR.bin", RESOURCE_TEXTURE2D); }, "비 DDS 입력 허용");
            Reject<std::invalid_argument>([&] { SharedDdsTexture::Load(nullptr, gpu.list.Get(), path, RESOURCE_TEXTURE2D); }, "null device 허용");
            Require(SharedDdsTexture::GetStatistics().created == beforeFailure.created, "실패 DDS 캐시 등록");
            Gpu second(true);
            auto isolated = SharedDdsTexture::Load(second.device.Get(), second.list.Get(), path, RESOURCE_TEXTURE2D);
            Require(isolated->Resource() != a->Resource(), "device 간 GPU 텍스처 공유");
            second.Execute(); isolated.reset(); second.Diagnostics();
            gpu.Execute();
            VerifyPixels(gpu, *a, path);
            a->ReleaseUpload(); b->ReleaseUpload(); Require(!b->Upload(), "공용 upload 반복 해제 실패");
            auto completed = SharedDdsTexture::Load(gpu.device.Get(), anotherList.Get(), path, RESOURCE_TEXTURE2D);
            Require(completed == a, "완료 후 list 공유 실패");
            std::weak_ptr<SharedDdsTexture> weak = a;
            a.reset(); b.reset(); Require(!weak.expired(), "남은 소유자 texture 조기 해제");
            completed.reset(); Require(weak.expired(), "weak cache가 자원 유지");
            auto created = SharedDdsTexture::GetStatistics().created;
            gpu.Reset(); auto recreated = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), path, RESOURCE_TEXTURE2D);
            Require(SharedDdsTexture::GetStatistics().created == created + 1, "만료 DDS 재생성 실패");
            gpu.Execute();
        }
        Require(SharedDdsTexture::GetStatistics().live == 0, "DDS 수명 검사 종료 후 잔류");

        gnCbvSrvDescriptorIncrementSize = gpu.device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
        CScene::CreateCbvSrvDescriptorHeaps(gpu.device.Get(), 0, 256);
        ComPtr<ID3D12DescriptorHeap> descriptorOwner; descriptorOwner.Attach(CScene::m_pd3dCbvSrvDescriptorHeap);
        UINT64 defaultBytes = 0, uploadBytes = 0;
        std::unique_ptr<CTexture> survivor;
        {
            gpu.Reset(); auto stats = SharedDdsTexture::GetStatistics();
            Model chest{CGameObject::LoadGeometryAndAnimationFromFile(gpu.device.Get(), gpu.list.Get(), nullptr, "Model/Monsters/Normal/ChestMonsterPBR.bin", nullptr)};
            Model beholder{CGameObject::LoadGeometryAndAnimationFromFile(gpu.device.Get(), gpu.list.Get(), nullptr, "Model/Monsters/Normal/BeholderPBR.bin", nullptr)};
            std::vector<CTexture*> c, b; Textures(chest.info->m_pModelRootObject, c); Textures(beholder.info->m_pModelRootObject, b);
            Require(c.size() == 2 && b.size() == 2, "실제 Chest/Beholder texture 수 변경");
            Require(SharedDdsTexture::GetStatistics().created == stats.created + 2, "실제 모델 DDS 중복 생성");
            for (auto texture : c)
            {
                auto peer = std::find_if(b.begin(), b.end(), [&](CTexture* other) { return other->GetResource(0) == texture->GetResource(0); });
                Require(peer != b.end() && *peer != texture, "실제 모델 GPU 텍스처 공유 실패");
                const auto originalHandle = texture->GetGpuDescriptorHandle(0);
                Require((*peer)->GetGpuDescriptorHandle(0).ptr == originalHandle.ptr, "동일 힙 읽기 전용 SRV 재사용 실패");
                (*peer)->SetGpuDescriptorHandle(0, {});
                Require(texture->GetGpuDescriptorHandle(0).ptr == originalHandle.ptr, "SRV handle 저장 공간 공유 오류");
                (*peer)->SetGpuDescriptorHandle(0, originalHandle);
                const auto originalRoot = texture->GetRootParameterIndex(0);
                (*peer)->SetRootParameterIndex(0, originalRoot + 10);
                Require(texture->GetRootParameterIndex(0) == originalRoot, "다른 모델 root parameter 덮어쓰기");
                (*peer)->SetRootParameterIndex(0, originalRoot);
                auto desc = texture->GetResource(0)->GetDesc();
                defaultBytes += gpu.device->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
                auto owner = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), texture->GetTextureType() == RESOURCE_TEXTURE2D && originalRoot == 3 ? path : L"Model/Textures/MSPBR.dds", RESOURCE_TEXTURE2D);
                uploadBytes += owner->Upload()->GetDesc().Width;
            }
            const auto format = c[0]->GetResource(0)->GetDesc().Format;
            const UINT64 expectedBytes = (format == DXGI_FORMAT_BC7_UNORM ? 2ull : 8ull) * 1024 * 1024;
            Require(defaultBytes == expectedBytes && uploadBytes == defaultBytes, "실제 DDS 할당량 변경");
            gpu.Execute();
            auto copy = std::make_unique<CTexture>(*c[0]);
            copy->SetRootParameterIndex(0, 42);
            Require(c[0]->GetRootParameterIndex(0) != 42, "복사 wrapper root 상태 공유");
            chest.info->m_pModelRootObject->ReleaseUploadBuffers();
            beholder.info->m_pModelRootObject->ReleaseUploadBuffers();
            beholder.info->m_pModelRootObject->ReleaseUploadBuffers();
            auto owner = SharedDdsTexture::Load(gpu.device.Get(), gpu.list.Get(), path, RESOURCE_TEXTURE2D);
            Require(owner->Upload() == nullptr, "실제 모델 upload 반복 해제 실패");
            // 새 장면 힙에서는 descriptor를 다시 생성해야 한다. 이전 힙 handle 재사용을 막는다.
            const auto oldHandle = c[0]->GetGpuDescriptorHandle(0);
            const auto oldRoot = c[0]->GetRootParameterIndex(0);
            CScene::CreateCbvSrvDescriptorHeaps(gpu.device.Get(), 0, 64);
            ComPtr<ID3D12DescriptorHeap> replacement; replacement.Attach(CScene::m_pd3dCbvSrvDescriptorHeap);
            CScene::CreateShaderResourceViews(gpu.device.Get(), c[0], 0, oldRoot);
            Require(c[0]->GetGpuDescriptorHandle(0).ptr != oldHandle.ptr, "장면 변경 후 이전 SRV 힙 handle 재사용");
            descriptorOwner = std::move(replacement);
            survivor = std::move(copy);
        }
        CScene::m_pd3dCbvSrvDescriptorHeap = nullptr;
        Require(SharedDdsTexture::GetStatistics().live == 1 && survivor->GetResource(0)->GetDesc().Width == 1024,
            "모델 해제 후 복사 wrapper GPU 자원 수명 실패");
        survivor.reset();
        Require(SharedDdsTexture::GetStatistics().live == 0, "실제 모델 종료 후 DDS 잔류");
        const auto warnings = gpu.Diagnostics();
        report << "{\"ok\":true,\"checks\":12,\"assetTextures\":4,\"uniqueResources\":2,\"savedDefaultBytes\":" << defaultBytes
            << ",\"savedUploadBytes\":" << uploadBytes << ",\"liveAfterExit\":0,\"gpuErrors\":0,\"gpuWarnings\":" << warnings << ",\"gpuWarningIds\":{";
        bool first = true;
        for (const auto& [id, count] : gpu.warningIds) { if (!first) report << ','; first = false; report << '"' << id << "\":" << count; }
        report << "}}";
        return 0;
    }
    catch (const std::exception& error)
    {
        report << "{\"ok\":false}";
        std::ofstream(std::filesystem::path(std::wstring(reportPath) + L".error.txt")) << error.what();
        return 1;
    }
}
