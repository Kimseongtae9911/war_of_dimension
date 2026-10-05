#include "stdafx.h"
#include "GameFramework.h"
#include "ModelPartSelection.h"
#include "NetworkManager.h"
#include <filesystem>
#include <stdexcept>
#include <cstring>

struct HeroSelectionTestAccess
{
    static void AddPlayer(NetworkManager& network, SC_ADD_PLAYER_PACKET& packet)
    { network.AddPlayerPacket(0, reinterpret_cast<BASE_PACKET*>(&packet)); }
};

namespace
{
    void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    void Frames(CGameObject* root, std::vector<CGameObject*>& frames)
    {
        for (auto frame = root; frame; frame = frame->m_pSibling)
        {
            frames.push_back(frame);
            if (frame->m_pChild) Frames(frame->m_pChild, frames);
        }
    }
    unsigned long long CompareModels(CLoadedModelInfo* full, CLoadedModelInfo* subset, const ModelPartSelection& selection)
    {
        std::vector<CGameObject*> before, after;
        Frames(full->m_pModelRootObject, before); Frames(subset->m_pModelRootObject, after);
        Require(before.size() == 788 && before.size() == after.size(), "프레임 계층 보존 실패");
        for (size_t i = 0; i < before.size(); ++i)
        {
            auto old = before[i], current = after[i];
            Require(!strcmp(old->m_pstrFrameName, current->m_pstrFrameName), "프레임 이름/순서 변경");
            Require(!memcmp(&old->m_xmf4x4ToParent, &current->m_xmf4x4ToParent, sizeof(XMFLOAT4X4)), "초기 transform 변경");
            if (!selection.Includes(old->m_pstrFrameName))
                Require(!current->m_pMesh && current->m_nMaterials == 0, "미선택 파츠 자원 생성");
            else
            {
                Require(bool(old->m_pMesh) == bool(current->m_pMesh), "선택 메시 누락");
                if (auto skin = dynamic_cast<CSkinnedMesh*>(current->m_pMesh))
                {
                    Require(skin != old->m_pMesh, "본 wrapper 상태 공유 오류");
                    for (int bone = 0; bone < skin->m_nSkinningBones; ++bone)
                        Require(skin->m_ppSkinningBoneFrameCaches[bone] != nullptr, "본 프레임 연결 누락");
                }
                const UINT textureBits[]{MATERIAL_ALBEDO_MAP, MATERIAL_SPECULAR_MAP, MATERIAL_NORMAL_MAP,
                                         MATERIAL_METALLIC_MAP, MATERIAL_EMISSION_MAP, MATERIAL_DETAIL_ALBEDO_MAP, MATERIAL_DETAIL_NORMAL_MAP};
                for (int slot = 0; slot < current->m_nMaterials; ++slot)
                    for (int texture = 0; texture < 7; ++texture)
                        if (current->m_ppMaterials[slot]->m_nType & textureBits[texture])
                            Require(current->m_ppMaterials[slot]->m_ppTextures[texture] != nullptr, "선택 파츠의 texture 참조 누락");
            }
        }
        auto a = full->m_pAnimationSets, b = subset->m_pAnimationSets;
        Require(a->m_nAnimationSets == 61 && a->m_nAnimationSets == b->m_nAnimationSets, "애니메이션 clip/index 변경");
        std::vector<int> indices;
        for (int i = 0; i < a->m_nAnimatedBoneFrames; ++i)
        {
            auto selectedFrame = subset->m_pModelRootObject->FindFrame(a->m_ppAnimatedBoneFrameCaches[i]->m_pstrFrameName);
            if (selectedFrame->m_bLoadAnimationFrame) indices.push_back(i);
        }
        Require(indices.size() == b->m_nAnimatedBoneFrames && indices.size() < a->m_nAnimatedBoneFrames, "애니메이션 프레임 remap 오류");
        unsigned long long compared = 0;
        for (int clip = 0; clip < a->m_nAnimationSets; ++clip)
        {
            auto old = a->m_pAnimationSets[clip], current = b->m_pAnimationSets[clip];
            Require(old->m_nKeyFrames == current->m_nKeyFrames && old->m_fLength == current->m_fLength, "애니메이션 key/길이 변경");
            for (int key = 0; key < old->m_nKeyFrames; ++key)
                for (size_t frame = 0; frame < indices.size(); ++frame)
                {
                    Require(!memcmp(&old->m_ppxmf4x4KeyFrameTransforms[key][indices[frame]],
                                    &current->m_ppxmf4x4KeyFrameTransforms[key][frame], sizeof(XMFLOAT4X4)), "애니메이션 행렬 내용 변경");
                    ++compared;
                }
        }
        return compared;
    }
    void RejectBadRecords()
    {
        auto token = [](FILE* file, const char* tag) {
            auto count = static_cast<unsigned char>(strlen(tag)); fwrite(&count, 1, 1, file); fwrite(tag, 1, count, file);
        };
        for (int test = 0; test < 3; ++test)
        {
            FILE* raw = nullptr; Require(tmpfile_s(&raw) == 0 && raw, "검증용 임시 파일 실패");
            std::unique_ptr<FILE, decltype(&fclose)> file(raw, fclose);
            if (test < 2)
            {
                token(raw, "skin"); token(raw, test == 0 ? "<BoneOffsets>:" : "<BonesPerVertex>:");
                int count = test == 0 ? 5 : -1; fwrite(&count, sizeof(count), 1, raw);
            }
            else { int count = 1; fwrite(&count, sizeof(count), 1, raw); token(raw, "<Unknown>:"); }
            rewind(raw);
            bool rejected = false;
            try { if (test < 2) SkipModelSkinInfo(raw); else SkipModelMaterials(raw); }
            catch (const std::runtime_error&) { rejected = true; }
            Require(rejected, "잘못된 선택 파츠 레코드 허용");
        }
    }
}

int CGameFramework::AuditHeroSelection(const wchar_t* reportPath)
{
    auto network = NetworkManager::GetInstance();
    Require(!network->GetFrozenIngameAppearances(), "미수신 외형 fallback 오류");
    network->SeedTestIngameAppearances();
    auto female = network->m_ArrayInGameClientsCustom[1]; female.Chr_Sex = 1;
    auto different = network->m_ArrayInGameClientsCustom[2]; different.Chr_Torso = 5;
    const std::array<ModelCustomize, 3> fixtures{network->m_ArrayInGameClientsCustom[0], female, different};
    network->Reset(); network->SetId(0); network->playerScene = SCENEKIND::READY;
    for (int slot = 0; slot < 3; ++slot)
    {
        SC_ADD_PLAYER_PACKET packet{}; packet.id = slot; packet.model = fixtures[slot];
        HeroSelectionTestAccess::AddPlayer(*network, packet);
    }
    network->FreezeIngameAppearances();
    Require(network->GetFrozenIngameAppearances().has_value(), "모델 생성 전 패킷 외형 수신 누락");
    const auto snapshot = *network->GetFrozenIngameAppearances();
    auto later = snapshot[0]; later.Chr_Torso = 7;
    network->StoreIngameAppearance(0, later);
    Require(network->GetFrozenIngameAppearance(0)->Chr_Torso == snapshot[0].Chr_Torso, "확정 외형의 사후 변경");
    ModelPartSelection unionParts(snapshot), ownParts(std::span<const ModelCustomize>(&snapshot[0], 1));
    Require(unionParts.Includes("Chr_Torso_Female_03") && unionParts.Includes("Chr_Torso_Male_05"), "남녀/다른 외형 합집합 누락");
    Require(!ownParts.Includes("Chr_Torso_Female_03") && !ownParts.Includes("Chr_Torso_Male_05"), "로컬 파츠 필터 오류");
    Require(ownParts.Includes("Hips") && ownParts.Includes("SM_bow_creep") && ownParts.Includes("UnknownFutureMesh"), "공통 본/무기/fallback 누락");
    RejectBadRecords();
    Require(SUCCEEDED(m_pd3dCommandList->Reset(m_pd3dCommandAllocator, nullptr)), "검증 command list reset 실패");
    auto full = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", nullptr);
    auto other = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", nullptr, &unionParts);
    auto own = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", nullptr, &ownParts);
    Require(SUCCEEDED(m_pd3dCommandList->Close()), "검증 command list close 실패");
    ID3D12CommandList* lists[]{m_pd3dCommandList}; m_pd3dCommandQueue->ExecuteCommandLists(1, lists); WaitForGpuComplete();
    const auto compared = CompareModels(full, other, unionParts) + CompareModels(full, own, ownParts);
    Require(full->m_nSkinnedMeshes == 720 && other->m_nSkinnedMeshes + own->m_nSkinnedMeshes == 91, "실제 에셋 스킨 수 불일치");
    const int unionSkins = other->m_nSkinnedMeshes, ownSkins = own->m_nSkinnedMeshes;
    const int unionFrames = other->m_pAnimationSets->m_nAnimatedBoneFrames, ownFrames = own->m_pAnimationSets->m_nAnimatedBoneFrames;
    for (auto model : {full, other, own})
    {
        model->m_pModelRootObject->ReleaseUploadBuffers();
        model->m_pAnimationSets->Release();
        model->m_pModelRootObject->Release();
        delete model;
    }
    network->Reset(); Require(!network->GetFrozenIngameAppearances(), "다음 매치 snapshot reset 실패");
    std::ofstream report{std::filesystem::path(reportPath)};
    report << "{\"ok\":true,\"backend\":\"D3D12 hardware\",\"unionSkins\":" << unionSkins
           << ",\"ownSkins\":" << ownSkins << ",\"unionAnimatedFrames\":" << unionFrames
           << ",\"ownAnimatedFrames\":" << ownFrames << ",\"comparedMatrixRows\":" << compared
           << ",\"checks\":[\"appearance_snapshot\",\"packet_before_models\",\"reset_and_fallback\",\"male_female_union\",\"local_selection\",\"skeleton_weapons_unknown_names\",\"malformed_records\",\"frame_hierarchy\",\"skin_bone_links\",\"texture_references\",\"all_clip_key_matrices\",\"gpu_complete_upload_release\"]}\n";
    Require(bool(report), "영웅 검증 결과 기록 실패");
    return 0;
}
