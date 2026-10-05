#pragma once
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <mutex>

// 모델 material 전용 읽기 전용 DDS 자원. SRV/root parameter는 CTexture별로 유지한다.
class SharedDdsTexture
{
public:
    ~SharedDdsTexture();
    ID3D12Resource* Resource() const { return m_resource.Get(); }
    ID3D12Resource* Upload() const;
    void ReleaseUpload(); // 호출자가 GPU 복사 완료를 보장한다. 반복 호출 가능.
    void CheckUploadList(ID3D12GraphicsCommandList* commands) const;
    static std::shared_ptr<SharedDdsTexture> Load(ID3D12Device* device,
        ID3D12GraphicsCommandList* commands, const wchar_t* path, UINT resourceType);
    struct Statistics { size_t created = 0, hits = 0, live = 0;
        UINT64 defaultBytes = 0, uploadBytes = 0; size_t uploads = 0; };
    static Statistics GetStatistics();

private:
    SharedDdsTexture(ID3D12Device* device, ID3D12GraphicsCommandList* commands, const wchar_t* path);
    Microsoft::WRL::ComPtr<ID3D12Device> m_device;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_resource, m_upload;
    mutable std::mutex m_uploadMutex;
    ID3D12GraphicsCommandList* m_uploadList = nullptr; // 비소유. upload 해제 시 제거한다.
    UINT64 m_defaultBytes = 0, m_uploadBytes = 0;
};
