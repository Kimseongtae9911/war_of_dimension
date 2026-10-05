#include "stdafx.h"
#include "GameFramework.h"
#include "ClientMemoryProfile.h"
#include "SharedDdsTexture.h"
#include "d3dx12.h"
#include <filesystem>
#include <wincodec.h>
#include <algorithm>
#include <stdexcept>
#pragma comment(lib, "windowscodecs.lib")

namespace
{
    using Microsoft::WRL::ComPtr;
    void Check(HRESULT value) { if (FAILED(value)) throw std::runtime_error("몬스터 GPU 촬영 실패"); }
    class CaptureCamera : public CCamera
    {
    public:
        void UpdateShaderVariables(ID3D12GraphicsCommandList* list) override
        {
            CCamera::UpdateShaderVariables(list);
            m_pcbMappedCamera->m_xm4x4ShadowTransform = Matrix4x4::Identity();
        }
    };
    BoundingBox WorldBounds(CGameObject* object)
    {
        BoundingBox result{};
        bool found = false;
        std::function<void(CGameObject*)> visit = [&](CGameObject* frame) {
            if (!frame) return;
            if (frame->m_pMesh) {
                BoundingBox world;
                world = frame->m_pMesh->GetPoseWorldBounds(frame->m_xmf4x4World);
                if (found) BoundingBox::CreateMerged(result, result, world);
                else { result = world; found = true; }
            }
            visit(frame->m_pChild); visit(frame->m_pSibling);
        };
        visit(object);
        if (!found) throw std::runtime_error("촬영 대상 메시 없음");
        return result;
    }
    void SavePng(const std::filesystem::path& file, UINT width, UINT height, UINT pitch, BYTE* pixels)
    {
        ComPtr<IWICImagingFactory> factory;
        ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapEncoder> encoder;
        ComPtr<IWICBitmapFrameEncode> frame;
        Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
        Check(factory->CreateStream(&stream));
        Check(stream->InitializeFromFilename(file.c_str(), GENERIC_WRITE));
        Check(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
        Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
        Check(encoder->CreateNewFrame(&frame, nullptr));
        Check(frame->Initialize(nullptr)); Check(frame->SetSize(width, height));
        // WIC PNG encoder의 표준 BGRA 입력으로 변환한다. GPU 원본은 RGBA다.
        std::vector<BYTE> bgra(static_cast<size_t>(width) * height * 4);
        for (UINT y = 0; y < height; ++y) for (UINT x = 0; x < width; ++x) {
            const auto source = pixels + y * pitch + x * 4;
            const auto target = bgra.data() + (static_cast<size_t>(y) * width + x) * 4;
            target[0] = source[2]; target[1] = source[1]; target[2] = source[0]; target[3] = 255;
        }
        auto format = GUID_WICPixelFormat32bppBGRA;
        Check(frame->SetPixelFormat(&format));
        if (format != GUID_WICPixelFormat32bppBGRA) throw std::runtime_error("PNG BGRA 포맷 미지원");
        Check(frame->WritePixels(height, width * 4, width * height * 4, bgra.data()));
        Check(frame->Commit()); Check(encoder->Commit());
    }
}

// 실제 인게임 모델·재질·Deferred 셰이더를 고정 포즈/카메라로 렌더링한다.
void CGameFramework::CaptureMonsters(const wchar_t* directory)
{
    auto scene = dynamic_cast<CIngameScene*>(m_pScene);
    if (!scene) throw std::runtime_error("몬스터 촬영은 인게임 초기화 이후 실행");
    ClientMemoryProfileRecordGpuDiagnostics(m_pd3dDevice, "initialization");
    ComPtr<ID3D12InfoQueue> diagnostics;
    if (SUCCEEDED(m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&diagnostics)))) diagnostics->ClearStoredMessages();
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) Check(com);
    CaptureCamera camera;
    camera.CreateShaderVariables(m_pd3dDevice, m_pd3dCommandList);
    camera.GenerateProjectionMatrix(0.1f, 10000.0f, ASPECT_RATIO, 40.0f);
    const char* names[]{"red", "green", "golem", "bear", "minotaur", "chest", "beholder", "minion", "ogre"};
    CGameObject* subjects[]{scene->m_monsters[0], scene->m_monsters[1], scene->m_monsters[2],
        scene->m_monsters[3], scene->m_monsters[4], scene->m_monsters[5], scene->m_monsters[6],
        scene->m_minions[0], scene->m_ppOtherClient[3]};
    std::set<CGameObject*> visited;
    std::set<std::shared_ptr<SharedDdsTexture>> textures;
    std::function<void(CGameObject*)> collect = [&](CGameObject* frame) {
        if (!frame || !visited.insert(frame).second) return;
        for (int material = 0; material < frame->m_nMaterials; ++material) {
            auto value = frame->m_ppMaterials[material]; if (!value) continue;
            for (int texture = 0; texture < value->m_nTextures; ++texture) {
                auto wrapper = value->m_ppTextures[texture]; if (!wrapper) continue;
                for (int slot = 0; slot < wrapper->GetTextures(); ++slot)
                    if (auto owner = wrapper->GetSharedDds(slot)) textures.insert(owner);
            }
        }
        collect(frame->m_pChild); collect(frame->m_pSibling);
    };
    for (auto subject : subjects) collect(subject);
    UINT64 textureAllocation = 0;
    for (const auto& owner : textures) {
        if (owner->Upload()) throw std::runtime_error("NPC DDS upload 조기 해제 누락");
        const auto description = owner->Resource()->GetDesc();
        textureAllocation += m_pd3dDevice->GetResourceAllocationInfo(0, 1, &description).SizeInBytes;
    }
    // 같은 geometry/DDS를 여러 경로에서 다시 방문해도 DEFAULT 자원은 유지되어야 한다.
    scene->ReleaseUploadBuffers(); scene->ReleaseUploadBuffers();
    for (const auto& owner : textures) if (owner->Upload() || !owner->Resource())
        throw std::runtime_error("NPC 반복 upload 해제 실패");
    const auto constants = scene->m_objectConstantArena->GetStatistics();
    std::ofstream manifest(std::filesystem::path(directory) / "capture.json");
    manifest << "{\"width\":1920,\"height\":1080,\"fov\":40,\"poseTime\":0,\"renderer\":\"game-deferred\",\"npcTextureResources\":"
        << textures.size() << ",\"npcTextureAllocationBytes\":" << textureAllocation
        << ",\"npcUploadsAfterRelease\":0,\"objectConstantSlots\":" << constants.slots
        << ",\"objectConstantPages\":" << constants.pages << ",\"objectConstantBytes\":" << constants.bytes << ",\"views\":[";
    bool first = true;
    for (size_t index = 0; index < std::size(subjects); ++index)
    {
        auto object = subjects[index];
        object->DrawOn(); object->SetPosition(0, 0, 0); object->SetObjectType(OBJ_TYPE::TEXTURE);
        object->m_nObjectDissolveState = 0;
        if (auto animation = object->m_pSkinnedAnimationController) {
            for (int track = 0; track < animation->m_nAnimationTracks; ++track) {
                animation->SetTrackEnable(track, track == 0); animation->SetTrackPosition(track, 0);
            }
        }
        object->Animate(0); object->UpdateTransform(nullptr);
        const auto bounds = WorldBounds(object);
        const float radius = (std::max)(0.1f, std::sqrt(bounds.Extents.x * bounds.Extents.x +
            bounds.Extents.y * bounds.Extents.y + bounds.Extents.z * bounds.Extents.z));
        const float distance = radius * 1.15f / std::sin(XMConvertToRadians(20.0f));
        for (int view = 0; view < 2; ++view)
        {
            const float direction = view ? -1.0f : 1.0f;
            XMFLOAT3 eye{bounds.Center.x + distance * 0.32f * direction,
                bounds.Center.y + distance * 0.18f, bounds.Center.z + distance * 0.93f * direction};
            camera.GenerateViewMatrix(eye, bounds.Center, XMFLOAT3(0, 1, 0));
            WaitForGpuComplete();
            Check(m_pd3dCommandAllocator->Reset()); Check(m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr));
            scene->BeginObjectConstantFrame(m_nSwapChainBufferIndex);
            auto buffer = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
            auto rtv = m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex];
            SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            SynchronizeResourceTransition(m_pd3dCommandList, m_ShadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            m_pd3dCommandList->ClearDepthStencilView(m_ShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
            SynchronizeResourceTransition(m_pd3dCommandList, m_ShadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
            scene->OnPrepareRender(m_pd3dCommandList, &camera);
            UpdateShaderVariables(); m_pTime->fCurrentTime = 1; m_pTime->fElapsedTime = 0;
            const auto dsv = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
            m_pd3dCommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1, 0, 0, nullptr);
            m_pPostProcessingShader->OnPrepareRenderTarget(m_pd3dCommandList, 1, &rtv, dsv);
            object->Render(m_pd3dCommandList, &camera, object->iMyShareNum);
            m_pPostProcessingShader->OnPostRenderTarget(m_pd3dCommandList);
            m_pd3dCommandList->OMSetRenderTargets(1, &rtv, TRUE, &dsv);
            // Heap 교체 시 이전 모델 descriptor table도 무효화한다.
            m_pd3dCommandList->SetGraphicsRootSignature(nullptr);
            m_pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
            camera.UpdateShaderVariables(m_pd3dCommandList);
            scene->UpdateShaderVariables(m_pd3dCommandList);
            m_pd3dCommandList->SetGraphicsRootConstantBufferView(18, m_pd3dcbTime->GetGPUVirtualAddress());
            m_pd3dCommandList->SetDescriptorHeaps(1, &m_pPostProcessingShader->m_pd3dCbvSrvDescriptorHeap);
            m_pd3dCommandList->SetGraphicsRootDescriptorTable(17, m_ShadowMap->Srv());
            m_pPostProcessingShader->Render(m_pd3dCommandList, &camera);
            const auto description = buffer->GetDesc();
            if (description.Format != DXGI_FORMAT_R8G8B8A8_UNORM) throw std::runtime_error("촬영 backbuffer RGBA 포맷 불일치");
            D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{}; UINT rows; UINT64 rowBytes, bytes;
            m_pd3dDevice->GetCopyableFootprints(&description, 0, 1, 0, &footprint, &rows, &rowBytes, &bytes);
            ComPtr<ID3D12Resource> readback;
            const auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
            const auto desc = CD3DX12_RESOURCE_DESC::Buffer(bytes);
            Check(m_pd3dDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &desc,
                D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
            SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
            const CD3DX12_TEXTURE_COPY_LOCATION source(buffer, 0), destination(readback.Get(), footprint);
            m_pd3dCommandList->CopyTextureRegion(&destination, 0, 0, 0, &source, nullptr);
            SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
            Check(m_pd3dCommandList->Close()); ID3D12CommandList* lists[]{m_pd3dCommandList};
            m_pd3dCommandQueue->ExecuteCommandLists(1, lists);
            scene->SubmitObjectConstantFrame(m_pd3dCommandQueue); WaitForGpuComplete();
            BYTE* mapped; const D3D12_RANGE range{0, static_cast<SIZE_T>(bytes)};
            Check(readback->Map(0, &range, reinterpret_cast<void**>(&mapped)));
            const std::string name = std::string(names[index]) + (view ? "-rear.png" : "-front.png");
            SavePng(std::filesystem::path(directory) / name, static_cast<UINT>(description.Width), description.Height,
                footprint.Footprint.RowPitch, mapped + footprint.Offset);
            const D3D12_RANGE written{0, 0}; readback->Unmap(0, &written);
            if (!first) manifest << ','; first = false;
            manifest << "{\"file\":\"" << name << "\",\"center\":[" << bounds.Center.x << ',' << bounds.Center.y << ',' << bounds.Center.z
                << "],\"eye\":[" << eye.x << ',' << eye.y << ',' << eye.z << "],\"distance\":" << distance << '}';
        }
    }
    manifest << "]}\n";
    ClientMemoryProfileRecordGpuDiagnostics(m_pd3dDevice, "capture-end");
    camera.ReleaseShaderVariables();
    if (SUCCEEDED(com)) CoUninitialize();
}
