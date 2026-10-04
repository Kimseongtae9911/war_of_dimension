#pragma once

#include "d3dx12.h"
#include <basetsd.h>

template<typename T>
class UploadBuffer
{
public:
	UploadBuffer(ID3D12Device* device, const UINT32 elementCount, const bool isConstantBuffer) :
		isConstantBuffer(isConstantBuffer)
	{
		elementByteSize = sizeof(T);

		// 상수 버퍼일 경우 elementByteSize는 256바이트의 배수여야 함
		if (isConstantBuffer)
			elementByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(T));

		// 업로드 버퍼 생성
		auto heap = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(elementByteSize * elementCount);
		ThrowIfFailed(device->CreateCommittedResource(
			&heap,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&uploadBuffer)));

		// 업로드 버퍼의 메모리 주소를 매핑
		ThrowIfFailed(uploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mappedData)));

		// 자원의 사용이 끝나기 전에는 해제할 필요가 없으나 
		// GPU가 사용중일 때 CPU가 자원 갱신을 하면 안된다. (동기화 필요)
	}

	UploadBuffer(const UploadBuffer& rhs) = delete;
	UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
	~UploadBuffer()
	{
		// 메모리 해제
		if (uploadBuffer != nullptr)
			uploadBuffer->Unmap(0, nullptr);

		mappedData = nullptr;
	}

	ID3D12Resource* Resource()const
	{
		return uploadBuffer.Get();
	}

	// 메모리에 데이터 복사
	void CopyData(int elementIndex, const T& data)
	{
		memcpy(&mappedData[elementIndex * elementByteSize], &data, sizeof(T));
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> uploadBuffer;
	BYTE* mappedData = nullptr;

	UINT32 elementByteSize = 0;
	bool isConstantBuffer = false;
};