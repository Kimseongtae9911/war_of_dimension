#include "stdafx.h"
#include "MeshSharingTests.h"
#include "MeshContent.h"
#include "Object.h"
#include <filesystem>
#include <stdexcept>

namespace
{
    using Microsoft::WRL::ComPtr;
    using File = std::unique_ptr<FILE, decltype(&fclose)>;
    void Require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
    void Check(HRESULT hr) { Require(SUCCEEDED(hr), "D3D12 테스트 자원 생성/실행 실패"); }

    File TemporaryFile()
    {
        FILE* file = nullptr;
        Require(tmpfile_s(&file) == 0 && file, "테스트 임시 파일 생성 실패");
        return File(file, fclose);
    }
    template<typename T> void Write(FILE* file, const T& data) { Require(fwrite(&data, sizeof(data), 1, file) == 1, "테스트 데이터 기록 실패"); }
    void Token(FILE* file, const std::string& value)
    {
        Write(file, static_cast<unsigned char>(value.size()));
        Require(fwrite(value.data(), 1, value.size(), file) == value.size(), "테스트 태그 기록 실패");
    }
    File MakeTestTriangle(const char* name, float position = 0, float uv = 0, bool reverse = false, float bounds = 1)
    {
        auto file = TemporaryFile();
        Write(file.get(), 3); Token(file.get(), name);
        Token(file.get(), "<Bounds>:");
        Write(file.get(), std::array<float, 6>{0, 0, 0, bounds, 1, 1});
        Token(file.get(), "<Positions>:"); Write(file.get(), 3);
        Write(file.get(), std::array<float, 9>{position, 0, 0, 1, 0, 0, 0, 1, 0});
        Token(file.get(), "<TextureCoords0>:"); Write(file.get(), 3);
        Write(file.get(), std::array<float, 6>{uv, 0, 1, 0, 0, 1});
        Token(file.get(), "<SubMeshes>:"); Write(file.get(), 1);
        Token(file.get(), "<SubMesh>:"); Write(file.get(), 0); Write(file.get(), 3);
        Write(file.get(), reverse ? std::array<unsigned int, 3>{0, 2, 1} : std::array<unsigned int, 3>{0, 1, 2});
        Token(file.get(), "</Mesh>"); rewind(file.get());
        return file;
    }
    File Skin(const char* bone)
    {
        auto file = TemporaryFile();
        Token(file.get(), "skin"); Token(file.get(), "<BonesPerVertex>:"); Write(file.get(), 4);
        Token(file.get(), "<BoneNames>:"); Write(file.get(), 1); Token(file.get(), bone);
        Token(file.get(), "<BoneOffsets>:"); Write(file.get(), 1); Write(file.get(), Matrix4x4::Identity());
        Token(file.get(), "<BoneIndices>:"); Write(file.get(), 3); Write(file.get(), std::array<int, 12>{});
        Token(file.get(), "<BoneWeights>:"); Write(file.get(), 3);
        Write(file.get(), std::array<float, 12>{1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0});
        Token(file.get(), "</SkinningInfo>"); rewind(file.get());
        return file;
    }
    class DeviceContext
    {
    public:
        ComPtr<ID3D12Device> device;
        ComPtr<ID3D12GraphicsCommandList> list;
        explicit DeviceContext(bool useWarp = true)
        {
            ComPtr<IDXGIFactory4> factory;
            ComPtr<IDXGIAdapter> warp;
            Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)));
            if (useWarp) Check(factory->EnumWarpAdapter(IID_PPV_ARGS(&warp)));
            Check(D3D12CreateDevice(warp.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device)));
            Check(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&allocator)));
            Check(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, allocator.Get(), nullptr, IID_PPV_ARGS(&list)));
            D3D12_COMMAND_QUEUE_DESC desc{};
            Check(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&queue)));
            Check(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        }
        void Complete()
        {
            Check(list->Close());
            ID3D12CommandList* lists[]{list.Get()};
            queue->ExecuteCommandLists(1, lists);
            Check(queue->Signal(fence.Get(), ++value));
            HANDLE event = CreateEvent(nullptr, FALSE, FALSE, nullptr);
            Require(event != nullptr, "GPU fence event 생성 실패");
            const auto hr = fence->SetEventOnCompletion(value, event);
            const auto waited = SUCCEEDED(hr) ? WaitForSingleObject(event, 30000) : WAIT_FAILED;
            CloseHandle(event);
            Check(hr); Require(waited == WAIT_OBJECT_0, "GPU fence 대기 실패");
            Check(allocator->Reset()); Check(list->Reset(allocator.Get(), nullptr));
        }
    private:
        ComPtr<ID3D12CommandAllocator> allocator;
        ComPtr<ID3D12CommandQueue> queue;
        ComPtr<ID3D12Fence> fence;
        UINT64 value = 0;
    };
    std::shared_ptr<CStandardMesh> Load(DeviceContext& context, FILE* file)
    {
        rewind(file);
        return CStandardMesh::LoadSharedGeometryFromFile(context.device.Get(), context.list.Get(), file);
    }
    void JsonString(std::ostream& stream, const std::string& value)
    {
        stream << '"';
        for (unsigned char c : value)
            if (c == '"' || c == '\\') stream << '\\' << c;
            else if (c < 32) stream << '?';
            else stream << c;
        stream << '"';
    }
    void FailedReport(const wchar_t* path, const char* error)
    {
        std::ofstream report{std::filesystem::path(path)};
        report << "{\"ok\":false,\"error\":"; JsonString(report, error); report << "}\n";
    }
}

int RunMeshSharingTests(const wchar_t* reportPath)
{
    try
    {
        // 같은 adapter에 대한 D3D12CreateDevice는 같은 device를 반환한다.
        DeviceContext first, second(false);
        Require(first.device.Get() != second.device.Get(), "device 경계 테스트용 하드웨어 adapter가 없습니다.");
        auto originalFile = MakeTestTriangle("stone");
        auto renamedFile = MakeTestTriangle("stone_copy");
        auto positionFile = MakeTestTriangle("stone", 0.25f);
        auto uvFile = MakeTestTriangle("stone", 0, 0.25f);
        auto indexFile = MakeTestTriangle("stone", 0, 0, true);
        auto boundsFile = MakeTestTriangle("stone", 0, 0, false, 2);
        auto original = Load(first, originalFile.get());
        auto renamed = Load(first, renamedFile.get());
        Require(original == renamed, "이름만 다른 geometry를 중복 생성했습니다.");
        Require(original->GetPositionBufferAddress() != 0, "실제 GPU 정점 버퍼가 없습니다.");
        Require(_ftelli64(renamedFile.get()) == _filelengthi64(_fileno(renamedFile.get())), "캐시 hit 이후 파일 위치가 잘못되었습니다.");
        auto position = Load(first, positionFile.get());
        auto uv = Load(first, uvFile.get());
        auto index = Load(first, indexFile.get());
        auto bounds = Load(first, boundsFile.get());
        Require(original != position && original != uv && original != index && original != bounds, "서로 다른 geometry를 잘못 합쳤습니다.");
        auto otherDevice = Load(second, originalFile.get());
        Require(original != otherDevice, "서로 다른 device의 GPU 자원을 공유했습니다.");
        auto concurrentFileA = MakeTestTriangle("parallel_a", 0.75f);
        auto concurrentFileB = MakeTestTriangle("parallel_b", 0.75f);
        std::array<std::shared_ptr<CStandardMesh>, 2> concurrent;
        std::array<std::exception_ptr, 2> failures{};
        const auto beforeConcurrent = CStandardMesh::GetGeometryCacheStatistics();
        std::thread workerA([&] { try { concurrent[0] = Load(first, concurrentFileA.get()); } catch (...) { failures[0] = std::current_exception(); } });
        std::thread workerB([&] { try { concurrent[1] = Load(first, concurrentFileB.get()); } catch (...) { failures[1] = std::current_exception(); } });
        workerA.join(); workerB.join();
        for (const auto& failure : failures) if (failure) std::rethrow_exception(failure);
        Require(concurrent[0] == concurrent[1] && CStandardMesh::GetGeometryCacheStatistics().created == beforeConcurrent.created + 1, "동시 로딩에서 geometry를 중복 생성했습니다.");
        CGameObject a, b;
        strcpy_s(a.m_pstrFrameName, "stone_instance_1"); strcpy_s(b.m_pstrFrameName, "stone_instance_2");
        a.SetSharedMesh(original); b.SetSharedMesh(renamed); b.SetSharedMesh(b.m_sharedMesh);
        a.SetPosition(1, 2, 3); b.SetPosition(9, 8, 7); a.SetScale(2, 3, 4);
        Require(a.m_pMesh == b.m_pMesh && a.GetPosition().x == 1 && b.GetPosition().x == 9 && b.GetScale().x == 1, "공유 후 transform이 간섭했습니다.");
        auto skinA = std::make_unique<CSkinnedMesh>(first.device.Get(), first.list.Get());
        auto skinB = std::make_unique<CSkinnedMesh>(first.device.Get(), first.list.Get());
        auto skinFileA = Skin("bone_a"), skinFileB = Skin("bone_b");
        skinA->LoadSkinInfoFromFile(first.device.Get(), first.list.Get(), skinFileA.get());
        skinB->LoadSkinInfoFromFile(first.device.Get(), first.list.Get(), skinFileB.get());
        rewind(originalFile.get()); rewind(renamedFile.get());
        skinA->LoadSharedGeometryDataFromFile(first.device.Get(), first.list.Get(), originalFile.get());
        skinB->LoadSharedGeometryDataFromFile(first.device.Get(), first.list.Get(), renamedFile.get());
        CGameObject boneA, boneB;
        strcpy_s(boneA.m_pstrFrameName, "bone_a"); strcpy_s(boneB.m_pstrFrameName, "bone_b");
        skinA->PrepareSkinning(&boneA); skinB->PrepareSkinning(&boneB);
        Require(skinA->GetGeometryOwner() == original.get() && skinB->GetPositionBufferAddress() == original->GetPositionBufferAddress(), "스킨드 base geometry 공유 실패");
        Require(skinA->m_pd3dcbBindPoseBoneOffsets != skinB->m_pd3dcbBindPoseBoneOffsets && skinA->m_ppSkinningBoneFrameCaches[0] == &boneA && skinB->m_ppSkinningBoneFrameCaches[0] == &boneB, "본 연결/스킨 상태가 간섭했습니다.");
        Require(strcmp(skinA->m_pstrMeshName, "stone") == 0 && strcmp(skinB->m_pstrMeshName, "stone_copy") == 0, "스킨드 메시 이름이 바뀌었습니다.");
        auto truncated = TemporaryFile(); Write(truncated.get(), 3); Token(truncated.get(), "broken"); rewind(truncated.get());
        bool rejected = false;
        try { Load(first, truncated.get()); } catch (const std::exception&) { rejected = true; }
        Require(rejected, "잘린 메시를 거부하지 않았습니다.");
        auto invalid = TemporaryFile(); Write(invalid.get(), -1); rewind(invalid.get());
        rejected = false;
        try { Load(first, invalid.get()); } catch (const std::exception&) { rejected = true; }
        Require(rejected, "음수 정점 수를 거부하지 않았습니다.");
        first.Complete(); second.Complete();
        a.ReleaseUploadBuffers(); b.ReleaseUploadBuffers(); skinA->ReleaseUploadBuffers(); skinB->ReleaseUploadBuffers();
        for (const auto& mesh : {position, uv, index, bounds, otherDevice}) mesh->ReleaseUploadBuffers();
        concurrent[0]->ReleaseUploadBuffers(); concurrent[1]->ReleaseUploadBuffers();
        std::weak_ptr<CStandardMesh> lifetime = original;
        original.reset(); renamed.reset(); a.SetMesh(nullptr);
        Require(!lifetime.expired(), "다른 객체가 사용하는 geometry를 일찍 해제했습니다.");
        skinA.reset(); skinB.reset(); b.SetMesh(nullptr);
        Require(lifetime.expired(), "마지막 소유자가 사라져도 geometry를 보관합니다.");
        const auto before = CStandardMesh::GetGeometryCacheStatistics();
        auto reloaded = Load(first, originalFile.get()); first.Complete(); reloaded->ReleaseUploadBuffers();
        Require(CStandardMesh::GetGeometryCacheStatistics().created == before.created + 1, "해제된 cache 항목을 재사용했습니다.");
        reloaded.reset(); position.reset(); uv.reset(); index.reset(); bounds.reset(); otherDevice.reset();
        concurrent[0].reset(); concurrent[1].reset();
        const auto stats = CStandardMesh::GetGeometryCacheStatistics();
        Require(stats.live == 0, "종료 후 공유 geometry가 남았습니다.");
        std::ofstream report{std::filesystem::path(reportPath)};
        Require(report.good(), "테스트 결과 파일 생성 실패");
        report << "{\"ok\":true,\"backend\":\"D3D12 WARP + hardware\",\"checks\":[\"renamed_mesh\",\"gpu_buffer_identity\",\"file_cursor\",\"geometry_changes\",\"device_isolation\",\"concurrent_load\",\"transform_isolation\",\"skinning_isolation\",\"malformed_input\",\"upload_release\",\"last_owner_release\",\"expired_cache_reload\"],\"created\":" << stats.created << ",\"reused\":" << stats.reused << ",\"live\":" << stats.live << "}\n";
        return 0;
    }
    catch (const std::exception& error) { FailedReport(reportPath, error.what()); return 1; }
}

int AuditMeshAssets(const wchar_t* reportPath)
{
    try
    {
        const std::vector<std::string> paths{"Model/LobbyScene_No.bin", "Model/Plane1.bin", "Model/ModularModel.bin"};
        const std::array<unsigned char, 8> marker{7, '<', 'M', 'e', 's', 'h', '>', ':'};
        std::ofstream report{std::filesystem::path(reportPath)};
        Require(report.good(), "에셋 점검 결과 파일 생성 실패");
        report << "{\"ok\":true,\"files\":[";
        bool first = true;
        for (const auto& path : paths)
        {
            FILE* raw = nullptr; fopen_s(&raw, path.c_str(), "rb"); File file(raw, fclose);
            Require(raw != nullptr, "실제 모델 파일이 없습니다.");
            std::map<std::array<unsigned char, 32>, MeshContentRecord> unique;
            std::map<std::array<unsigned char, 32>, std::shared_ptr<CStandardMesh>> retained;
            DeviceContext context;
            const auto before = CStandardMesh::GetGeometryCacheStatistics();
            size_t count = 0, renamedDuplicates = 0;
            std::uint64_t bytes = 0, uniqueBytes = 0;
            std::vector<std::pair<std::string, std::string>> aliases;
            std::array<unsigned char, 64 * 1024> chunk{};
            for (;;)
            {
                const auto begin = _ftelli64(raw);
                const size_t read = fread(chunk.data(), 1, chunk.size(), raw);
                if (!read) break;
                const auto found = std::search(chunk.begin(), chunk.begin() + read, marker.begin(), marker.end());
                if (found == chunk.begin() + read)
                {
                    if (read < chunk.size()) break;
                    Require(_fseeki64(raw, -static_cast<long long>(marker.size() - 1), SEEK_CUR) == 0, "에셋 검색 위치 복원 실패");
                    continue;
                }
                Require(_fseeki64(raw, begin + std::distance(chunk.begin(), found) + marker.size(), SEEK_SET) == 0, "에셋 메시 위치 복원 실패");
                const auto record = ReadMeshContentRecord(raw);
                Require(_fseeki64(raw, record.begin, SEEK_SET) == 0, "에셋 GPU 로딩 위치 복원 실패");
                const auto mesh = CStandardMesh::LoadSharedGeometryFromFile(context.device.Get(), context.list.Get(), raw);
                ++count; bytes += record.contentBytes;
                auto [entry, inserted] = unique.emplace(record.digest, record);
                if (inserted)
                {
                    uniqueBytes += record.contentBytes;
                    retained.emplace(record.digest, mesh);
                }
                else
                {
                    Require(retained.at(record.digest) == mesh, "실제 에셋에서 중복 GPU geometry를 생성했습니다.");
                    if (entry->second.name != record.name)
                    {
                        ++renamedDuplicates;
                        if (aliases.size() < 5) aliases.emplace_back(entry->second.name, record.name);
                    }
                }
            }
            Require(count != 0, "에셋에서 메시를 찾지 못했습니다.");
            context.Complete();
            for (const auto& entry : retained) entry.second->ReleaseUploadBuffers();
            const auto after = CStandardMesh::GetGeometryCacheStatistics();
            Require(after.created - before.created == unique.size() && after.reused - before.reused == count - unique.size(), "실제 에셋 GPU 생성/공유 횟수가 맞지 않습니다.");
            if (!first) report << ','; first = false;
            report << "{\"path\":"; JsonString(report, path);
            report << ",\"records\":" << count << ",\"uniqueGeometry\":" << unique.size() << ",\"duplicateRecords\":" << count - unique.size() << ",\"renamedDuplicates\":" << renamedDuplicates << ",\"gpuGeometryCreated\":" << after.created - before.created << ",\"gpuGeometryReused\":" << after.reused - before.reused << ",\"serializedBytes\":" << bytes << ",\"uniqueSerializedBytes\":" << uniqueBytes << ",\"aliases\":[";
            bool firstAlias = true;
            for (const auto& alias : aliases)
            {
                if (!firstAlias) report << ','; firstAlias = false;
                report << '['; JsonString(report, alias.first); report << ','; JsonString(report, alias.second); report << ']';
            }
            report << "]}";
            retained.clear();
            Require(CStandardMesh::GetGeometryCacheStatistics().live == 0, "실제 에셋 점검 종료 후 캐시가 자원을 보유합니다.");
        }
        report << "]}\n";
        return 0;
    }
    catch (const std::exception& error) { FailedReport(reportPath, error.what()); return 1; }
}
