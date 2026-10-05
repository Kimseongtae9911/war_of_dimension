#include "stdafx.h"
#include "GameFramework.h"
#include "ClientMemoryProfile.h"
#include "UiTextureLayout.h"
#include "d3dx12.h"
#include <filesystem>
#include <wincodec.h>
#include <stdexcept>
#pragma comment(lib, "windowscodecs.lib")

namespace
{
    using Microsoft::WRL::ComPtr;
    std::vector<ComPtr<ID3D12Resource>> lifetimeOwners;
    std::filesystem::path capturePath;
    void Check(HRESULT value) { if (FAILED(value)) throw std::runtime_error("UI GPU 캡처 실패"); }
    void SavePng(const std::filesystem::path& file, UINT width, UINT height, UINT pitch, const BYTE* pixels)
    {
        ComPtr<IWICImagingFactory> factory; ComPtr<IWICStream> stream;
        ComPtr<IWICBitmapEncoder> encoder; ComPtr<IWICBitmapFrameEncode> frame;
        Check(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory)));
        Check(factory->CreateStream(&stream)); Check(stream->InitializeFromFilename(file.c_str(), GENERIC_WRITE));
        Check(factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder));
        Check(encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache));
        Check(encoder->CreateNewFrame(&frame, nullptr)); Check(frame->Initialize(nullptr));
        Check(frame->SetSize(width, height)); auto format = GUID_WICPixelFormat32bppBGRA;
        Check(frame->SetPixelFormat(&format));
        if (format != GUID_WICPixelFormat32bppBGRA) throw std::runtime_error("PNG BGRA 형식 불일치");
        std::vector<BYTE> bgra(static_cast<size_t>(width) * height * 4);
        for (UINT y = 0; y < height; ++y) for (UINT x = 0; x < width; ++x) {
            const BYTE* src = pixels + y * pitch + x * 4;
            BYTE* dst = bgra.data() + (static_cast<size_t>(y) * width + x) * 4;
            dst[0] = src[2]; dst[1] = src[1]; dst[2] = src[0]; dst[3] = 255;
        }
        Check(frame->WritePixels(height, width * 4, width * height * 4, bgra.data()));
        Check(frame->Commit()); Check(encoder->Commit());
    }
    void CheckLayoutFailures()
    {
        std::array<uint8_t, 148> data{};
        D3D12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_BC7_UNORM, 232, 156, 1, 1);
        const uint32_t valid[]{0x37424955u, 1, 230, 153, 1, 1};
        const auto original = ReadUiTextureLayout(data.data(), desc);
        if (original.x != 1 || original.y != 1 || original.z != 0 || original.w != 0)
            throw std::runtime_error("비압축 DDS 기본 UV 계약 불일치");
        for (int invalid = 0; invalid < 9; ++invalid) {
            std::memcpy(data.data() + 32, valid, sizeof(valid)); auto corrupt = desc;
            uint32_t words[6]; std::memcpy(words, valid, sizeof(words));
            switch (invalid) {
            case 0: words[1] = 2; break;
            case 1: words[2] = 0; break;
            case 2: words[3] = 0; break;
            case 3: words[4] = UINT_MAX; break;
            case 4: words[5] = UINT_MAX; break;
            case 5: corrupt.MipLevels = 2; break;
            case 6: corrupt.Format = DXGI_FORMAT_R8G8B8A8_UNORM; break;
            case 7: corrupt.DepthOrArraySize = 2; break;
            case 8: words[4] = 0; break;
            }
            std::memcpy(data.data() + 32, words, sizeof(words));
            bool rejected = false;
            try { ReadUiTextureLayout(data.data(), corrupt); } catch (const std::runtime_error&) { rejected = true; }
            if (!rejected) throw std::runtime_error("잘못된 UI DDS 내용 계약 미거부");
        }
    }
}

// GPU 완료·Framework OnDestroy 이후 유일한 외부 검사 참조도 반환한다.
void CheckUiCaptureRelease()
{
    size_t retained = 0;
    for (auto& owner : lifetimeOwners) if (owner.Detach()->Release() != 0) ++retained;
    const size_t checked = lifetimeOwners.size(); lifetimeOwners.clear();
    std::ofstream report(capturePath / "ui-lifetime.json");
    report << "{\"checkedResources\":" << checked << ",\"resourcesRetainedAfterOnDestroy\":" << retained << "}\n";
    if (retained) throw std::runtime_error("UI DEFAULT/UPLOAD 종료 참조 잔존");
}

// 실제 INGAME CTextureShader의 UI 객체/재질/PSO로 고정 시각의 비교 화면을 만든다.
void CGameFramework::CaptureUiTextures(const wchar_t* directory)
{
    capturePath = directory; CheckLayoutFailures();
    auto shader = CTextureShader::GetInstance();
    if (!shader || shader->m_CurScene != SCENEKIND::INGAME) throw std::runtime_error("인게임 UI 생성 전 캡처");
    ClientMemoryProfileRecordGpuDiagnostics(m_pd3dDevice, "initialization");
    ComPtr<ID3D12InfoQueue> diagnostics;
    if (SUCCEEDED(m_pd3dDevice->QueryInterface(IID_PPV_ARGS(&diagnostics)))) diagnostics->ClearStoredMessages();
    const HRESULT com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(com) && com != RPC_E_CHANGED_MODE) Check(com);
    std::vector<CUIObject*> all;
    auto append = [&](CUIObject** objects, int count) { for (int i = 0; i < count; ++i) if (objects[i]) all.push_back(objects[i]); };
    append(shader->m_ppObjects, shader->m_nObjects); append(shader->m_ppUITextures, shader->m_nUITextures);
    append(shader->m_ppExtraTextures, shader->m_nExtraTextures);
    all.insert(all.end(), shader->m_shopTextures.begin(), shader->m_shopTextures.end());
    all.insert(all.end(), shader->m_ItemTextures.begin(), shader->m_ItemTextures.end());
    all.insert(all.end(), {shader->m_pAccelerateObject, shader->m_pGetDamageObject, shader->m_pWinTexture, shader->m_pDefeatTexture});
    std::set<ID3D12Resource*> unique;
    UINT64 defaults = 0, uploads = 0; unsigned int padded = 0;
    std::ofstream inventory(capturePath / "allocations.json"); inventory << "{\"resources\":[";
    bool first = true;
    for (auto object : all) {
        if (!object) continue;
        auto texture = object->m_ppMaterials[0]->m_ppTextures[0];
        auto resource = texture->GetResource(0);
        if (!unique.insert(resource).second) continue;
        const auto desc = resource->GetDesc();
        const auto allocation = m_pd3dDevice->GetResourceAllocationInfo(0, 1, &desc).SizeInBytes;
        defaults += allocation; lifetimeOwners.emplace_back(resource);
        const auto transform = texture->GetUiUvTransform();
        if (transform.x != 1 || transform.y != 1) ++padded;
        {
            CTexture copy(*texture);
            const auto cloned = copy.GetUiUvTransform();
            if (copy.GetResource(0) != resource || cloned.x != transform.x || cloned.y != transform.y ||
                cloned.z != transform.z || cloned.w != transform.w)
                throw std::runtime_error("UI texture 복사 참조/내용 좌표 보존 실패");
        }
        UINT64 uploadAllocation = 0;
        if (auto upload = texture->GetUploadResource(0)) {
            auto uploadDesc = upload->GetDesc();
            uploadAllocation = m_pd3dDevice->GetResourceAllocationInfo(0, 1, &uploadDesc).SizeInBytes;
            uploads += uploadAllocation; lifetimeOwners.emplace_back(upload);
        }
        if (!first) inventory << ','; first = false;
        inventory << "{\"width\":" << desc.Width << ",\"height\":" << desc.Height << ",\"format\":" << desc.Format
            << ",\"mips\":" << desc.MipLevels << ",\"defaultAllocationBytes\":" << allocation
            << ",\"uploadAllocationBytes\":" << uploadAllocation << ",\"uvTransform\":["
            << transform.x << ',' << transform.y << ',' << transform.z << ',' << transform.w << "]}";
    }
    inventory << "],\"uniqueTextures\":" << unique.size() << ",\"defaultAllocationBytes\":" << defaults
        << ",\"uploadAllocationBytes\":" << uploads << ",\"paddedTextures\":" << padded << ",\"layoutFailureCases\":9}\n";
    if (unique.size() != 20) throw std::runtime_error("역할별 UI 고유 texture 20개 계약 불일치");
    std::ofstream captures(capturePath / "capture.json"); captures << "{\"views\":[";
    bool firstView = true;
    auto render = [&](const std::string& name, const std::vector<CUIObject*>& subjects, float background) {
        WaitForGpuComplete(); Check(m_pd3dCommandAllocator->Reset());
        Check(m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr));
        auto buffer = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
        auto rtv = m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex];
        SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        const float clear[]{background, background, background, 1}; m_pd3dCommandList->ClearRenderTargetView(rtv, clear, 0, nullptr);
        m_pd3dCommandList->OMSetRenderTargets(1, &rtv, TRUE, nullptr);
        m_pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
        m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList); m_pCamera->UpdateShaderVariables(m_pd3dCommandList);
        m_pd3dCommandList->SetDescriptorHeaps(1, &CScene::m_pd3dCbvSrvDescriptorHeap);
        UpdateShaderVariables(); m_pTime->fCurrentTime = 1; m_pTime->fElapsedTime = 0;
        m_pScene->UpdateShaderVariables(m_pd3dCommandList);
        shader->CStandardShader::Render(m_pd3dCommandList, m_pCamera);
        for (auto object : subjects) {
            const bool visible = object->m_bIsRender; object->DrawOn();
            object->Render(m_pd3dCommandList, m_pCamera); object->m_bIsRender = visible;
        }
        const auto desc = buffer->GetDesc();
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint{}; UINT rows; UINT64 rowBytes, bytes;
        m_pd3dDevice->GetCopyableFootprints(&desc, 0, 1, 0, &footprint, &rows, &rowBytes, &bytes);
        ComPtr<ID3D12Resource> readback; const auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);
        const auto readbackDesc = CD3DX12_RESOURCE_DESC::Buffer(bytes);
        Check(m_pd3dDevice->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE, &readbackDesc,
            D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readback)));
        SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
        const CD3DX12_TEXTURE_COPY_LOCATION src(buffer, 0), dst(readback.Get(), footprint);
        m_pd3dCommandList->CopyTextureRegion(&dst, 0, 0, 0, &src, nullptr);
        SynchronizeResourceTransition(m_pd3dCommandList, buffer, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_PRESENT);
        Check(m_pd3dCommandList->Close()); ID3D12CommandList* lists[]{m_pd3dCommandList};
        ClientMemoryProfileRecordGpuDiagnostics(m_pd3dDevice, "ui-recorded");
        m_pd3dCommandQueue->ExecuteCommandLists(1, lists); WaitForGpuComplete();
        BYTE* mapped; const D3D12_RANGE range{0, static_cast<SIZE_T>(bytes)};
        Check(readback->Map(0, &range, reinterpret_cast<void**>(&mapped)));
        SavePng(capturePath / name, static_cast<UINT>(desc.Width), desc.Height, footprint.Footprint.RowPitch, mapped + footprint.Offset);
        const D3D12_RANGE written{0, 0}; readback->Unmap(0, &written);
        if (!firstView) captures << ','; firstView = false;
        captures << "{\"file\":\"" << name << "\",\"background\":" << background << '}';
    };
    std::vector<CUIObject*> hud;
    for (int i = 0; i < shader->m_nUITextures; ++i) hud.push_back(shader->m_ppUITextures[i]);
    for (int i = 0; i < shader->m_nObjects; ++i) hud.push_back(shader->m_ppObjects[i]);
    hud.push_back(shader->m_ppExtraTextures[0]);
    render("hud.png", hud, 0.18f); render("shop.png", shader->m_shopTextures, 0.18f);
    for (auto [name, object] : std::initializer_list<std::pair<const char*, CUIObject*>>{
        {"victory", shader->m_pWinTexture}, {"defeat", shader->m_pDefeatTexture}, {"blood", shader->m_pGetDamageObject}}) {
        render(std::string(name) + "-dark.png", {object}, 0.08f);
        render(std::string(name) + "-light.png", {object}, 0.75f);
    }
    auto speed = dynamic_cast<CTexturedRectMesh*>(shader->m_pAccelerateObject->m_pMesh);
    const auto oldSpeedUv = speed->m_fTexutreUV;
    for (int frame = 0; frame < 4; ++frame) {
        speed->SetUV({frame / 4.0f, 0});
        render("speed-" + std::to_string(frame) + ".png", {shader->m_pAccelerateObject}, 0.18f);
    }
    speed->SetUV(oldSpeedUv);
    // 작은 icon/button과 item atlas 전체를 같은 shader로 확대 비교한다.
    std::vector<CUIObject*> grid;
    Check(m_pd3dCommandAllocator->Reset()); Check(m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr));
    std::set<CTexture*> gridTextures;
    for (auto object : all) {
        auto texture = object->m_ppMaterials[0]->m_ppTextures[0];
        auto desc = texture->GetResource(0)->GetDesc();
        if (desc.Width > 240 || !gridTextures.insert(texture).second) continue;
        const int i = static_cast<int>(grid.size());
        grid.push_back(new CUIObject(m_pd3dDevice, m_pd3dCommandList, texture, {230, 156},
            {80.0f + (i % 7) * 255.0f, 80.0f + (i / 7) * 200.0f}, TEXTURETYPE::NONE, 1, {}));
    }
    for (int item = 0; item < 11; ++item) {
        auto texture = shader->m_ItemTextures[0]->m_ppMaterials[0]->m_ppTextures[0];
        grid.push_back(new CUIObject(m_pd3dDevice, m_pd3dCommandList, texture, {120, 120},
            {80.0f + (item % 11) * 160.0f, 560.0f}, TEXTURETYPE::ITEMKIND, 1, {item / 11.0f, 0}));
    }
    Check(m_pd3dCommandList->Close()); ID3D12CommandList* lists[]{m_pd3dCommandList};
    m_pd3dCommandQueue->ExecuteCommandLists(1, lists); WaitForGpuComplete();
    render("icons-items-dark.png", grid, 0.08f); render("icons-items-light.png", grid, 0.75f);
    for (auto object : grid) object->Release();
    grid.clear();
    Check(m_pd3dCommandAllocator->Reset()); Check(m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr));
    const auto skillMesh = dynamic_cast<CTexturedRectMesh*>(shader->m_ppObjects[0]->m_pMesh);
    const bool boss = skillMesh->m_nType == static_cast<int>(TEXTURETYPE::BOSSSKILL);
    const int columns = boss ? 10 : 12, skillRows = boss ? 2 : 4;
    auto skillTexture = shader->m_ppObjects[0]->m_ppMaterials[0]->m_ppTextures[0];
    for (int row = 0; row < skillRows; ++row) for (int col = 0; col < columns; ++col)
        grid.push_back(new CUIObject(m_pd3dDevice, m_pd3dCommandList, skillTexture, {100, 100},
            {80.0f + col * 145.0f, 50.0f + row * 125.0f}, boss ? TEXTURETYPE::BOSSSKILL : TEXTURETYPE::PLAYERSKILL,
            1, {static_cast<float>(col) / columns, static_cast<float>(row) / skillRows}));
    for (int progress = 0; progress < 5; ++progress) for (int bar = 0; bar < 2; ++bar) {
        auto texture = shader->m_ppUITextures[bar]->m_ppMaterials[0]->m_ppTextures[0];
        grid.push_back(new CUIObject(m_pd3dDevice, m_pd3dCommandList, texture, {650, 38},
            {100.0f + bar * 850.0f, 650.0f + progress * 70.0f}, bar ? TEXTURETYPE::PROGRESSBAR : TEXTURETYPE::PROGRESSBARR,
            progress / 4.0f, {}));
    }
    Check(m_pd3dCommandList->Close()); m_pd3dCommandQueue->ExecuteCommandLists(1, lists); WaitForGpuComplete();
    render("skills-bars-dark.png", grid, 0.08f); render("skills-bars-light.png", grid, 0.75f);
    for (auto object : grid) object->Release();
    captures << "],\"fixedTime\":1,\"width\":1920,\"height\":1080}\n";
    // 기존 공통 자원의 품질 계약도 실제 생성된 resource에서 검사한다.
    if (m_ShadowMap->Resource()->GetDesc().Width != 8192 ||
        m_pPostProcessingShader->GetTextureResource(2)->GetDesc().Format != DXGI_FORMAT_R32G32B32A32_FLOAT)
        throw std::runtime_error("유지하기로 한 shadow/위치 RT 품질 계약 변경");
    ClientMemoryProfileRecordGpuDiagnostics(m_pd3dDevice, "ui-capture-end");
    if (SUCCEEDED(com)) CoUninitialize();
}
