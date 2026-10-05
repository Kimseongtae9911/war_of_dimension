//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Mesh.h"
#include "Object.h"
#include "MeshContent.h"
#include "ClientMemoryProfile.h"
#include <tuple>
#include <stdexcept>

namespace
{
	using GeometryKey = std::tuple<std::uintptr_t, std::array<unsigned char, 32>, std::uint64_t>;
	std::mutex geometryCacheMutex;
	std::map<GeometryKey, std::weak_ptr<CStandardMesh>> geometryCache;
	size_t geometryCreated = 0;
	size_t geometryReused = 0;

	void PruneGeometryCache()
	{
		for (auto it = geometryCache.begin(); it != geometryCache.end();)
			if (it->second.expired()) it = geometryCache.erase(it);
			else ++it;
	}
}

CMesh::CMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
}

CMesh::~CMesh()
{
	if (m_sharedGeometry) return;
	CMesh::ReleaseUploadBuffers();
	if (m_pd3dPositionBuffer) m_pd3dPositionBuffer->Release();

	if (m_nSubMeshes > 0)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexBuffers[i]) m_ppd3dSubSetIndexBuffers[i]->Release();
			if (m_ppnSubSetIndices[i]) {
				delete[] m_ppnSubSetIndices[i];
				m_ppnSubSetIndices[i] = nullptr;
			}
		}
		if (m_ppd3dSubSetIndexBuffers) {
			delete[] m_ppd3dSubSetIndexBuffers;
			m_ppd3dSubSetIndexBuffers = nullptr;
		}
		if (m_pd3dSubSetIndexBufferViews) {
			delete[] m_pd3dSubSetIndexBufferViews;
			m_pd3dSubSetIndexBufferViews = nullptr;
		}

		if (m_pnSubSetIndices) {
			delete[] m_pnSubSetIndices;
			m_pnSubSetIndices = nullptr;
		}
		if (m_ppnSubSetIndices) {
			delete[] m_ppnSubSetIndices;
			m_ppnSubSetIndices = nullptr;
		}
	}

	if (m_pxmf3Positions) {
		delete[] m_pxmf3Positions;
		m_pxmf3Positions = nullptr;
	}
}

void CMesh::ReleaseUploadBuffers()
{
	if (m_sharedGeometry) { m_sharedGeometry->ReleaseUploadBuffers(); return; }
	if (m_pd3dPositionUploadBuffer) m_pd3dPositionUploadBuffer->Release();
	m_pd3dPositionUploadBuffer = NULL;

	if ((m_nSubMeshes > 0) && m_ppd3dSubSetIndexUploadBuffers)
	{
		for (int i = 0; i < m_nSubMeshes; i++)
		{
			if (m_ppd3dSubSetIndexUploadBuffers[i]) m_ppd3dSubSetIndexUploadBuffers[i]->Release();
		}
		if (m_ppd3dSubSetIndexUploadBuffers) delete[] m_ppd3dSubSetIndexUploadBuffers;
		m_ppd3dSubSetIndexUploadBuffers = NULL;
	}
}

void CMesh::OnPreRender(ID3D12GraphicsCommandList *pd3dCommandList, void *pContext)
{
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dPositionBufferView);
}

void CMesh::Render(ID3D12GraphicsCommandList *pd3dCommandList, int nSubSet)
{
	UpdateShaderVariables(pd3dCommandList);

	OnPreRender(pd3dCommandList, NULL);

	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);

	if((m_nSubMeshes > 0) && (nSubSet < m_nSubMeshes))
	{
		pd3dCommandList->IASetIndexBuffer(&(m_pd3dSubSetIndexBufferViews[nSubSet]));
		pd3dCommandList->DrawIndexedInstanced(m_pnSubSetIndices[nSubSet], 1, 0, 0, 0);
	}
	else
	{
		pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
	}
}

void CMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList)
{
	pd3dCommandList->IASetPrimitiveTopology(m_d3dPrimitiveTopology);
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 1, &m_d3dPositionBufferView);
	pd3dCommandList->DrawInstanced(m_nVertices, 1, m_nOffset, 0);
}

void CMesh::OnPostRender(ID3D12GraphicsCommandList *pd3dCommandList, void *pContext)
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CSkyBoxMesh::CSkyBoxMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, float fWidth, float fHeight, float fDepth) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = 36;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_pxmf3Positions = new XMFLOAT3[m_nVertices];

	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;
	// Front Quad (quads point inward)
	m_pxmf3Positions[0] = XMFLOAT3(-fx, +fx, +fx);
	m_pxmf3Positions[1] = XMFLOAT3(+fx, +fx, +fx);
	m_pxmf3Positions[2] = XMFLOAT3(-fx, -fx, +fx);
	m_pxmf3Positions[3] = XMFLOAT3(-fx, -fx, +fx);
	m_pxmf3Positions[4] = XMFLOAT3(+fx, +fx, +fx);
	m_pxmf3Positions[5] = XMFLOAT3(+fx, -fx, +fx);
	// Back Quad										
	m_pxmf3Positions[6] = XMFLOAT3(+fx, +fx, -fx);
	m_pxmf3Positions[7] = XMFLOAT3(-fx, +fx, -fx);
	m_pxmf3Positions[8] = XMFLOAT3(+fx, -fx, -fx);
	m_pxmf3Positions[9] = XMFLOAT3(+fx, -fx, -fx);
	m_pxmf3Positions[10] = XMFLOAT3(-fx, +fx, -fx);
	m_pxmf3Positions[11] = XMFLOAT3(-fx, -fx, -fx);
	// Left Quad										
	m_pxmf3Positions[12] = XMFLOAT3(-fx, +fx, -fx);
	m_pxmf3Positions[13] = XMFLOAT3(-fx, +fx, +fx);
	m_pxmf3Positions[14] = XMFLOAT3(-fx, -fx, -fx);
	m_pxmf3Positions[15] = XMFLOAT3(-fx, -fx, -fx);
	m_pxmf3Positions[16] = XMFLOAT3(-fx, +fx, +fx);
	m_pxmf3Positions[17] = XMFLOAT3(-fx, -fx, +fx);
	// Right Quad										
	m_pxmf3Positions[18] = XMFLOAT3(+fx, +fx, +fx);
	m_pxmf3Positions[19] = XMFLOAT3(+fx, +fx, -fx);
	m_pxmf3Positions[20] = XMFLOAT3(+fx, -fx, +fx);
	m_pxmf3Positions[21] = XMFLOAT3(+fx, -fx, +fx);
	m_pxmf3Positions[22] = XMFLOAT3(+fx, +fx, -fx);
	m_pxmf3Positions[23] = XMFLOAT3(+fx, -fx, -fx);
	// Top Quad											
	m_pxmf3Positions[24] = XMFLOAT3(-fx, +fx, -fx);
	m_pxmf3Positions[25] = XMFLOAT3(+fx, +fx, -fx);
	m_pxmf3Positions[26] = XMFLOAT3(-fx, +fx, +fx);
	m_pxmf3Positions[27] = XMFLOAT3(-fx, +fx, +fx);
	m_pxmf3Positions[28] = XMFLOAT3(+fx, +fx, -fx);
	m_pxmf3Positions[29] = XMFLOAT3(+fx, +fx, +fx);
	// Bottom Quad										
	m_pxmf3Positions[30] = XMFLOAT3(-fx, -fx, +fx);
	m_pxmf3Positions[31] = XMFLOAT3(+fx, -fx, +fx);
	m_pxmf3Positions[32] = XMFLOAT3(-fx, -fx, -fx);
	m_pxmf3Positions[33] = XMFLOAT3(-fx, -fx, -fx);
	m_pxmf3Positions[34] = XMFLOAT3(+fx, -fx, +fx);
	m_pxmf3Positions[35] = XMFLOAT3(+fx, -fx, -fx);

	m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
}

CSkyBoxMesh::~CSkyBoxMesh()
{
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
CStandardMesh::CStandardMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_d3dPositionBufferView.BufferLocation = 0;
	m_d3dPositionBufferView.StrideInBytes = 0;
	m_d3dPositionBufferView.SizeInBytes = 0;

	m_d3dTextureCoord0BufferView.BufferLocation = 0;
	m_d3dTextureCoord0BufferView.StrideInBytes = 0;
	m_d3dTextureCoord0BufferView.SizeInBytes = 0;
	m_d3dTextureCoord1BufferView = {};

	m_d3dNormalBufferView.BufferLocation = 0;
	m_d3dNormalBufferView.StrideInBytes = 0;
	m_d3dNormalBufferView.SizeInBytes = 0;

	m_d3dTangentBufferView.BufferLocation = 0;
	m_d3dTangentBufferView.StrideInBytes = 0;
	m_d3dTangentBufferView.SizeInBytes = 0;

	m_d3dBiTangentBufferView.BufferLocation = 0;
	m_d3dBiTangentBufferView.StrideInBytes = 0;
	m_d3dBiTangentBufferView.SizeInBytes = 0;
}

CStandardMesh::~CStandardMesh()
{
	if (m_sharedGeometry) return;
	CStandardMesh::ReleaseUploadBuffers();
	if (m_pd3dTextureCoord0Buffer) m_pd3dTextureCoord0Buffer->Release();
	if (m_pd3dNormalBuffer) m_pd3dNormalBuffer->Release();
	if (m_pd3dTangentBuffer) m_pd3dTangentBuffer->Release();
	if (m_pd3dBiTangentBuffer) m_pd3dBiTangentBuffer->Release();

	if (m_pxmf4Colors) delete[] m_pxmf4Colors;
	if (m_pxmf3Normals) delete[] m_pxmf3Normals;
	if (m_pxmf3Tangents) delete[] m_pxmf3Tangents;
	if (m_pxmf3BiTangents) delete[] m_pxmf3BiTangents;
	if (m_pxmf2TextureCoords0) delete[] m_pxmf2TextureCoords0;
	if (m_pxmf2TextureCoords1) delete[] m_pxmf2TextureCoords1;
}

void CStandardMesh::ReleaseUploadBuffers()
{
	if (m_sharedGeometry) { m_sharedGeometry->ReleaseUploadBuffers(); return; }
	CMesh::ReleaseUploadBuffers();

	if (m_pd3dTextureCoord0UploadBuffer) m_pd3dTextureCoord0UploadBuffer->Release();
	m_pd3dTextureCoord0UploadBuffer = NULL;

	if (m_pd3dNormalUploadBuffer) m_pd3dNormalUploadBuffer->Release();
	m_pd3dNormalUploadBuffer = NULL;

	if (m_pd3dTangentUploadBuffer) m_pd3dTangentUploadBuffer->Release();
	m_pd3dTangentUploadBuffer = NULL;

	if (m_pd3dBiTangentUploadBuffer) m_pd3dBiTangentUploadBuffer->Release();
	m_pd3dBiTangentUploadBuffer = NULL;
}

std::shared_ptr<CStandardMesh> CStandardMesh::LoadSharedGeometryFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, FILE* file, std::string* meshName)
{
	const auto record = ReadMeshContentRecord(file);
	if (meshName) *meshName = record.name;
	const GeometryKey key{ reinterpret_cast<std::uintptr_t>(device), record.digest, record.contentBytes };
	std::lock_guard<std::mutex> lock(geometryCacheMutex);
	if (((geometryCreated + geometryReused) & 255) == 0) PruneGeometryCache();
	if (auto found = geometryCache.find(key); found != geometryCache.end())
	{
		if (auto mesh = found->second.lock()) { ++geometryReused; return mesh; }
	}
	if (_fseeki64(file, record.begin, SEEK_SET) != 0) throw std::runtime_error("메시 파일 위치 복원 실패");
	auto mesh = new CStandardMesh(device, commandList);
	mesh->AddRef();
	std::shared_ptr<CStandardMesh> owner(mesh, [](CStandardMesh* resource) { resource->Release(); });
	mesh->LoadMeshFromFile(device, commandList, file);
	if (_ftelli64(file) != record.end) throw std::runtime_error("메시 파서와 GPU 로더의 위치가 다릅니다.");
	geometryCache[key] = owner;
	++geometryCreated;
	return owner;
}

CStandardMesh::CacheStatistics CStandardMesh::GetGeometryCacheStatistics()
{
	std::lock_guard<std::mutex> lock(geometryCacheMutex);
	PruneGeometryCache();
	return { geometryCreated, geometryReused, geometryCache.size() };
}

void CStandardMesh::LoadSharedGeometryDataFromFile(ID3D12Device* device, ID3D12GraphicsCommandList* commandList, FILE* file)
{
	if (m_sharedGeometry || m_pd3dPositionBuffer) throw std::runtime_error("이미 초기화한 geometry는 교체할 수 없습니다.");
	std::string meshName;
	m_sharedGeometry = LoadSharedGeometryFromFile(device, commandList, file, &meshName);
	strcpy_s(m_pstrMeshName, meshName.c_str());
	const auto& source = *m_sharedGeometry;
	m_nType |= source.m_nType;
	m_nVertices = source.m_nVertices;
	m_xmf3AABBExtents = source.m_xmf3AABBExtents;
	m_nSubMeshes = source.m_nSubMeshes;
	m_pxmf3Positions = source.m_pxmf3Positions;
	m_pd3dPositionBuffer = source.m_pd3dPositionBuffer;
	m_d3dPositionBufferView = source.m_d3dPositionBufferView;
	m_pnSubSetIndices = source.m_pnSubSetIndices;
	m_ppnSubSetIndices = source.m_ppnSubSetIndices;
	m_ppd3dSubSetIndexBuffers = source.m_ppd3dSubSetIndexBuffers;
	m_pd3dSubSetIndexBufferViews = source.m_pd3dSubSetIndexBufferViews;
	m_pxmf4Colors = source.m_pxmf4Colors;
	m_pxmf3Normals = source.m_pxmf3Normals;
	m_pxmf3Tangents = source.m_pxmf3Tangents;
	m_pxmf3BiTangents = source.m_pxmf3BiTangents;
	m_pxmf2TextureCoords0 = source.m_pxmf2TextureCoords0;
	m_pxmf2TextureCoords1 = source.m_pxmf2TextureCoords1;
	m_pd3dTextureCoord0Buffer = source.m_pd3dTextureCoord0Buffer;
	m_d3dTextureCoord0BufferView = source.m_d3dTextureCoord0BufferView;
	m_pd3dTextureCoord1Buffer = source.m_pd3dTextureCoord1Buffer;
	m_d3dTextureCoord1BufferView = source.m_d3dTextureCoord1BufferView;
	m_pd3dNormalBuffer = source.m_pd3dNormalBuffer;
	m_d3dNormalBufferView = source.m_d3dNormalBufferView;
	m_pd3dTangentBuffer = source.m_pd3dTangentBuffer;
	m_d3dTangentBufferView = source.m_d3dTangentBufferView;
	m_pd3dBiTangentBuffer = source.m_pd3dBiTangentBuffer;
	m_d3dBiTangentBufferView = source.m_d3dBiTangentBufferView;
}

void CStandardMesh::LoadMeshFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, FILE *pInFile)
{
	char pstrToken[64] = { '\0' };
	int nPositions = 0, nColors = 0, nNormals = 0, nTangents = 0, nBiTangents = 0, nTextureCoords = 0, nIndices = 0, nSubMeshes = 0, nSubIndices = 0;

	UINT nReads = (UINT)::fread(&m_nVertices, sizeof(int), 1, pInFile);

	::ReadStringFromFile(pInFile, m_pstrMeshName);

	XMFLOAT3 center;
	for ( ; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<Bounds>:"))
		{
			nReads = (UINT)::fread(&center, sizeof(XMFLOAT3), 1, pInFile);
			nReads = (UINT)::fread(&m_xmf3AABBExtents, sizeof(XMFLOAT3), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Positions>:"))
		{
			nReads = (UINT)::fread(&nPositions, sizeof(int), 1, pInFile);
			if (nPositions > 0)
			{
				m_nType |= VERTEXT_POSITION;
				m_pxmf3Positions = new XMFLOAT3[nPositions];
				nReads = (UINT)::fread(m_pxmf3Positions, sizeof(XMFLOAT3), nPositions, pInFile);

				m_pd3dPositionBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Positions, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

				m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
				m_d3dPositionBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dPositionBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<Colors>:"))
		{
			nReads = (UINT)::fread(&nColors, sizeof(int), 1, pInFile);
			if (nColors > 0)
			{
				m_nType |= VERTEXT_COLOR;
				m_pxmf4Colors = new XMFLOAT4[nColors];
				nReads = (UINT)::fread(m_pxmf4Colors, sizeof(XMFLOAT4), nColors, pInFile);
			}
		}
		else if (!strcmp(pstrToken, "<TextureCoords0>:"))
		{
			nReads = (UINT)::fread(&nTextureCoords, sizeof(int), 1, pInFile);
			if (nTextureCoords > 0)
			{
				m_nType |= VERTEXT_TEXTURE_COORD0;
				m_pxmf2TextureCoords0 = new XMFLOAT2[nTextureCoords];
				nReads = (UINT)::fread(m_pxmf2TextureCoords0, sizeof(XMFLOAT2), nTextureCoords, pInFile);

				m_pd3dTextureCoord0Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords0, sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord0UploadBuffer);

				m_d3dTextureCoord0BufferView.BufferLocation = m_pd3dTextureCoord0Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord0BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord0BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<TextureCoords1>:"))
		{
			nReads = (UINT)::fread(&nTextureCoords, sizeof(int), 1, pInFile);
			if (nTextureCoords > 0)
			{
				m_nType |= VERTEXT_TEXTURE_COORD1;
				m_pxmf2TextureCoords1 = new XMFLOAT2[nTextureCoords];
				nReads = (UINT)::fread(m_pxmf2TextureCoords1, sizeof(XMFLOAT2), nTextureCoords, pInFile);

				m_pd3dTextureCoord1Buffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf2TextureCoords1, sizeof(XMFLOAT2) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTextureCoord1UploadBuffer);

				m_d3dTextureCoord1BufferView.BufferLocation = m_pd3dTextureCoord1Buffer->GetGPUVirtualAddress();
				m_d3dTextureCoord1BufferView.StrideInBytes = sizeof(XMFLOAT2);
				m_d3dTextureCoord1BufferView.SizeInBytes = sizeof(XMFLOAT2) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<Normals>:"))
		{
			nReads = (UINT)::fread(&nNormals, sizeof(int), 1, pInFile);
			if (nNormals > 0)
			{
				m_nType |= VERTEXT_NORMAL;
				m_pxmf3Normals = new XMFLOAT3[nNormals];
				nReads = (UINT)::fread(m_pxmf3Normals, sizeof(XMFLOAT3), nNormals, pInFile);
				m_pd3dNormalBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Normals, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dNormalUploadBuffer);

				m_d3dNormalBufferView.BufferLocation = m_pd3dNormalBuffer->GetGPUVirtualAddress();
				m_d3dNormalBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dNormalBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<Tangents>:"))
		{
			nReads = (UINT)::fread(&nTangents, sizeof(int), 1, pInFile);
			if (nTangents > 0)
			{
				m_nType |= VERTEXT_TANGENT;
				m_pxmf3Tangents = new XMFLOAT3[nTangents];
				nReads = (UINT)::fread(m_pxmf3Tangents, sizeof(XMFLOAT3), nTangents, pInFile);

				m_pd3dTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3Tangents, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dTangentUploadBuffer);

				m_d3dTangentBufferView.BufferLocation = m_pd3dTangentBuffer->GetGPUVirtualAddress();
				m_d3dTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<BiTangents>:"))
		{
			nReads = (UINT)::fread(&nBiTangents, sizeof(int), 1, pInFile);
			if (nBiTangents > 0)
			{
				m_pxmf3BiTangents = new XMFLOAT3[nBiTangents];
				nReads = (UINT)::fread(m_pxmf3BiTangents, sizeof(XMFLOAT3), nBiTangents, pInFile);

				m_pd3dBiTangentBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf3BiTangents, sizeof(XMFLOAT3) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBiTangentUploadBuffer);

				m_d3dBiTangentBufferView.BufferLocation = m_pd3dBiTangentBuffer->GetGPUVirtualAddress();
				m_d3dBiTangentBufferView.StrideInBytes = sizeof(XMFLOAT3);
				m_d3dBiTangentBufferView.SizeInBytes = sizeof(XMFLOAT3) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<SubMeshes>:"))
		{
			nReads = (UINT)::fread(&(m_nSubMeshes), sizeof(int), 1, pInFile);
			if (m_nSubMeshes > 0)
			{
				m_pnSubSetIndices = new int[m_nSubMeshes];
				m_ppnSubSetIndices = new UINT*[m_nSubMeshes]();

				m_ppd3dSubSetIndexBuffers = new ID3D12Resource*[m_nSubMeshes]();
				m_ppd3dSubSetIndexUploadBuffers = new ID3D12Resource*[m_nSubMeshes]();
				m_pd3dSubSetIndexBufferViews = new D3D12_INDEX_BUFFER_VIEW[m_nSubMeshes]();

				for (int i = 0; i < m_nSubMeshes; i++)
				{
					::ReadStringFromFile(pInFile, pstrToken);
					if (!strcmp(pstrToken, "<SubMesh>:"))
					{
						int nIndex = 0;
						nReads = (UINT)::fread(&nIndex, sizeof(int), 1, pInFile); //i
						nReads = (UINT)::fread(&(m_pnSubSetIndices[i]), sizeof(int), 1, pInFile);
						if (m_pnSubSetIndices[i] > 0)
						{
							m_ppnSubSetIndices[i] = new UINT[m_pnSubSetIndices[i]];
							nReads = (UINT)::fread(m_ppnSubSetIndices[i], sizeof(UINT), m_pnSubSetIndices[i], pInFile);

							m_ppd3dSubSetIndexBuffers[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_ppnSubSetIndices[i], sizeof(UINT) * m_pnSubSetIndices[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, &m_ppd3dSubSetIndexUploadBuffers[i]);

							m_pd3dSubSetIndexBufferViews[i].BufferLocation = m_ppd3dSubSetIndexBuffers[i]->GetGPUVirtualAddress();
							m_pd3dSubSetIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
							m_pd3dSubSetIndexBufferViews[i].SizeInBytes = sizeof(UINT) * m_pnSubSetIndices[i];
						}
					}
				}
			}
		}
		else if (!strcmp(pstrToken, "</Mesh>"))
		{
			break;
		}
	}
}

void CStandardMesh::OnPreRender(ID3D12GraphicsCommandList *pd3dCommandList, void *pContext)
{
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[5] = { m_d3dPositionBufferView, m_d3dTextureCoord0BufferView, m_d3dNormalBufferView, m_d3dTangentBufferView, m_d3dBiTangentBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 5, pVertexBufferViews);
}

/////////////////////////////////////////////////////////////////////////////////////////////////
//
CSkinnedMesh::CSkinnedMesh(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) : CStandardMesh(pd3dDevice, pd3dCommandList)
{
}

CSkinnedMesh::~CSkinnedMesh()
{
	CSkinnedMesh::ReleaseUploadBuffers();
	if (m_pxmn4BoneIndices) {
		delete[] m_pxmn4BoneIndices;
		m_pxmn4BoneIndices = nullptr;
	}
	if (m_pxmf4BoneWeights) {
		delete[] m_pxmf4BoneWeights;
		m_pxmf4BoneWeights = nullptr;
	}

	if (m_ppSkinningBoneFrameCaches) {
		delete[] m_ppSkinningBoneFrameCaches;
		m_ppSkinningBoneFrameCaches = nullptr;
	}
	//if (m_ppstrSkinningBoneNames) {
	//	delete[] m_ppstrSkinningBoneNames;
	//	m_ppstrSkinningBoneNames = nullptr;
	//}

	for (char* name : m_ppstrSkinningBoneNames) {
		delete[] name;
	}
	m_pxmf4x4BindPoseBoneOffsets.clear();
	
	//if (m_pxmf4x4BindPoseBoneOffsets) {
	//	delete[] m_pxmf4x4BindPoseBoneOffsets;
	//	m_pxmf4x4BindPoseBoneOffsets = nullptr;
	//}

	if (m_pd3dBoneIndexBuffer) {
		m_pd3dBoneIndexBuffer->Release();
		m_pd3dBoneIndexBuffer = nullptr;
	}

	if (m_pd3dBoneWeightBuffer) {
		m_pd3dBoneWeightBuffer->Release();
		m_pd3dBoneWeightBuffer = nullptr;
	}

	ReleaseShaderVariables();
}

void CSkinnedMesh::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CSkinnedMesh::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pd3dcbBindPoseBoneOffsets)
	{
		D3D12_GPU_VIRTUAL_ADDRESS d3dcbBoneOffsetsGpuVirtualAddress = m_pd3dcbBindPoseBoneOffsets->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(11, d3dcbBoneOffsetsGpuVirtualAddress); //Skinned Bone Offsets
	}

	if (m_pd3dcbSkinningBoneTransforms)
	{
		D3D12_GPU_VIRTUAL_ADDRESS d3dcbBoneTransformsGpuVirtualAddress = m_pd3dcbSkinningBoneTransforms->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(12, d3dcbBoneTransformsGpuVirtualAddress); //Skinned Bone Transforms

		for (int j = 0; j < m_nSkinningBones; j++)
		{
			XMStoreFloat4x4(&m_pcbxmf4x4MappedSkinningBoneTransforms[j], XMMatrixTranspose(XMLoadFloat4x4(&m_ppSkinningBoneFrameCaches[j]->m_xmf4x4World)));
		}
	}
}

void CSkinnedMesh::ReleaseShaderVariables()
{
	if (m_pd3dcbBindPoseBoneOffsets)
	{
		m_pd3dcbBindPoseBoneOffsets->Unmap(0, nullptr);
		m_pd3dcbBindPoseBoneOffsets->Release();
		m_pd3dcbBindPoseBoneOffsets = nullptr;
		m_pcbxmf4x4MappedBindPoseBoneOffsets = nullptr;
	}
	// SkinningBoneTransforms는 animation controller가 소유한다.
}

void CSkinnedMesh::ReleaseUploadBuffers()
{
	CStandardMesh::ReleaseUploadBuffers();

	if (m_pd3dBoneIndexUploadBuffer) m_pd3dBoneIndexUploadBuffer->Release();
	m_pd3dBoneIndexUploadBuffer = NULL;

	if (m_pd3dBoneWeightUploadBuffer) m_pd3dBoneWeightUploadBuffer->Release();
	m_pd3dBoneWeightUploadBuffer = NULL;
}

void CSkinnedMesh::PrepareSkinning(CGameObject *pModelRootObject)
{
	for (int j = 0; j < m_nSkinningBones; j++)
	{
		m_ppSkinningBoneFrameCaches[j] = pModelRootObject->FindFrame(m_ppstrSkinningBoneNames[j]);
	}
}

void CSkinnedMesh::LoadSkinInfoFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, FILE *pInFile)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	::ReadStringFromFile(pInFile, m_pstrMeshName); 

	XMFLOAT3 center;
	for ( ; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<BonesPerVertex>:"))
		{
			m_nBonesPerVertex = ::ReadIntegerFromFile(pInFile);
		}
		else if (!strcmp(pstrToken, "<Bounds>:"))
		{
			nReads = (UINT)::fread(&center, sizeof(XMFLOAT3), 1, pInFile);
			nReads = (UINT)::fread(&m_xmf3AABBExtents, sizeof(XMFLOAT3), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<BoneNames>:"))
		{
			m_nSkinningBones = ::ReadIntegerFromFile(pInFile);
			if (m_nSkinningBones > 0) 
			{
				//m_ppstrSkinningBoneNames = new char[m_nSkinningBones][64];
				m_ppstrSkinningBoneNames.reserve(m_nSkinningBones);
				for (int i = 0; i < m_nSkinningBones; ++i) {
					m_ppstrSkinningBoneNames.push_back(nullptr);
				}
				m_ppSkinningBoneFrameCaches = new CGameObject*[m_nSkinningBones];
				for (int i = 0; i < m_nSkinningBones; i++)
				{
					m_ppstrSkinningBoneNames[i] = new char[64];
					::ReadStringFromFile(pInFile, m_ppstrSkinningBoneNames[i]);
					m_ppSkinningBoneFrameCaches[i] = NULL;
				}
			}
		}
		else if (!strcmp(pstrToken, "<BoneOffsets>:"))
		{
			m_nSkinningBones = ::ReadIntegerFromFile(pInFile);
			if (m_nSkinningBones > 0)
			{
				//m_pxmf4x4BindPoseBoneOffsets = new XMFLOAT4X4[m_nSkinningBones];
				for (int i = 0; i < m_nSkinningBones; ++i) {
					m_pxmf4x4BindPoseBoneOffsets.push_back(XMFLOAT4X4());
				}
				
				for (int i = 0; i < m_nSkinningBones; ++i) {
					nReads = (UINT)::fread((void*)&m_pxmf4x4BindPoseBoneOffsets[i], sizeof(XMFLOAT4X4), 1, pInFile);
				}

				UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
				m_pd3dcbBindPoseBoneOffsets = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
				m_pd3dcbBindPoseBoneOffsets->Map(0, NULL, (void **)&m_pcbxmf4x4MappedBindPoseBoneOffsets);

				for (int i = 0; i < m_nSkinningBones; i++)
				{
					XMStoreFloat4x4(&m_pcbxmf4x4MappedBindPoseBoneOffsets[i], XMMatrixTranspose(XMLoadFloat4x4(&m_pxmf4x4BindPoseBoneOffsets[i])));
				}
			}
		}
		else if (!strcmp(pstrToken, "<BoneIndices>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmn4BoneIndices = new XMINT4[m_nVertices];

				nReads = (UINT)::fread(m_pxmn4BoneIndices, sizeof(XMINT4), m_nVertices, pInFile);
				m_pd3dBoneIndexBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmn4BoneIndices, sizeof(XMINT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneIndexUploadBuffer);

				m_d3dBoneIndexBufferView.BufferLocation = m_pd3dBoneIndexBuffer->GetGPUVirtualAddress();
				m_d3dBoneIndexBufferView.StrideInBytes = sizeof(XMINT4);
				m_d3dBoneIndexBufferView.SizeInBytes = sizeof(XMINT4) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "<BoneWeights>:"))
		{
			m_nType |= VERTEXT_BONE_INDEX_WEIGHT;

			m_nVertices = ::ReadIntegerFromFile(pInFile);
			if (m_nVertices > 0)
			{
				m_pxmf4BoneWeights = new XMFLOAT4[m_nVertices];

				nReads = (UINT)::fread(m_pxmf4BoneWeights, sizeof(XMFLOAT4), m_nVertices, pInFile);
				m_pd3dBoneWeightBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, m_pxmf4BoneWeights, sizeof(XMFLOAT4) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dBoneWeightUploadBuffer);

				m_d3dBoneWeightBufferView.BufferLocation = m_pd3dBoneWeightBuffer->GetGPUVirtualAddress();
				m_d3dBoneWeightBufferView.StrideInBytes = sizeof(XMFLOAT4);
				m_d3dBoneWeightBufferView.SizeInBytes = sizeof(XMFLOAT4) * m_nVertices;
			}
		}
		else if (!strcmp(pstrToken, "</SkinningInfo>"))
		{
			break;
		}
	}
}

void CSkinnedMesh::OnPreRender(ID3D12GraphicsCommandList *pd3dCommandList, void *pContext)
{
	D3D12_VERTEX_BUFFER_VIEW pVertexBufferViews[7] = { m_d3dPositionBufferView, m_d3dTextureCoord0BufferView, m_d3dNormalBufferView, m_d3dTangentBufferView, m_d3dBiTangentBufferView, m_d3dBoneIndexBufferView, m_d3dBoneWeightBufferView };
	pd3dCommandList->IASetVertexBuffers(m_nSlot, 7, pVertexBufferViews);
}

void CTexturedRectMesh::Resize(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fResolution)
{
	// Update the member variables with the new size
	m_fWidth = m_fWidth * fResolution;
	m_fHeight = m_fHeight * fResolution;

	// Create a new array of vertices with the updated size
	CTexturedVertex pVertices[6];

	// Update the vertices with the new positions based on the new width and height
	float fx = (m_fWidth * 0.5f);
	float fy = (m_fHeight * 0.5f);

	pVertices[0] = CTexturedVertex(XMFLOAT3(fx, +fy, 0.0f), XMFLOAT2(1.0f, 0.0f)); 
	pVertices[1] = CTexturedVertex(XMFLOAT3(fx, -fy, 0.0f), XMFLOAT2(1.0f, 1.0f)); 
	pVertices[2] = CTexturedVertex(XMFLOAT3(-fx, -fy, 0.0f), XMFLOAT2(0.0f, 1.0f)); 
	pVertices[3] = CTexturedVertex(XMFLOAT3(-fx, -fy, 0.0f), XMFLOAT2(0.0f, 1.0f)); 
	pVertices[4] = CTexturedVertex(XMFLOAT3(-fx, +fy, 0.0f), XMFLOAT2(0.0f, 0.0f)); 
	pVertices[5] = CTexturedVertex(XMFLOAT3(fx, +fy, 0.0f), XMFLOAT2(1.0f, 0.0f)); 

	// Release the previous position buffer and upload buffer
	ReleaseUploadBuffers(); 

	// Recreate the buffer with the updated vertices
	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, sizeof(CTexturedVertex) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	// Update the buffer view with the new buffer's address and size
	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress(); 
	m_d3dPositionBufferView.SizeInBytes = sizeof(CTexturedVertex) * m_nVertices; 

}

void CTexturedRectMesh::ReleaseUploadBuffers()
{
	if (m_pd3dPositionUploadBuffer)
	{
		m_pd3dPositionUploadBuffer->Release();
		m_pd3dPositionUploadBuffer = nullptr;
	}
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CTexturedRectMesh::CTexturedRectMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth, float fxPosition, float fyPosition, float fzPosition) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_fWidth = fWidth;
	m_fHeight = fHeight;

	m_nVertices = 6;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	CTexturedVertex pVertices[6];

	float fx = (fWidth * 0.5f) + fxPosition, fy = (fHeight * 0.5f) + fyPosition, fz = (fDepth * 0.5f) + fzPosition;

	if (fWidth == 0.0f)
	{
		if (fxPosition > 0.0f)
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(fx, +fy, -fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(fx, -fy, -fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(fx, -fy, +fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(fx, -fy, +fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(fx, +fy, +fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(fx, +fy, -fz), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(fx, +fy, +fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(fx, -fy, +fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(fx, -fy, -fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(fx, -fy, -fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(fx, +fy, -fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(fx, +fy, +fz), XMFLOAT2(1.0f, 0.0f));
		}
	}
	else if (fHeight == 0.0f)
	{
		if (fyPosition > 0.0f)
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(+fx, fy, -fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(+fx, fy, +fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(-fx, fy, +fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(-fx, fy, +fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(-fx, fy, -fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(+fx, fy, -fz), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(+fx, fy, +fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(+fx, fy, -fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(-fx, fy, -fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(-fx, fy, -fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(-fx, fy, +fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(+fx, fy, +fz), XMFLOAT2(1.0f, 0.0f));
		}
	}
	else if (fDepth == 0.0f)
	{
		if (fzPosition > 0.0f)
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(+fx, +fy, fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(+fx, -fy, fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(-fx, -fy, fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(-fx, -fy, fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(-fx, +fy, fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(+fx, +fy, fz), XMFLOAT2(1.0f, 0.0f));
		}
		else
		{
			pVertices[0] = CTexturedVertex(XMFLOAT3(-fx, +fy, fz), XMFLOAT2(1.0f, 0.0f));
			pVertices[1] = CTexturedVertex(XMFLOAT3(-fx, -fy, fz), XMFLOAT2(1.0f, 1.0f));
			pVertices[2] = CTexturedVertex(XMFLOAT3(+fx, -fy, fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[3] = CTexturedVertex(XMFLOAT3(+fx, -fy, fz), XMFLOAT2(0.0f, 1.0f));
			pVertices[4] = CTexturedVertex(XMFLOAT3(+fx, +fy, fz), XMFLOAT2(0.0f, 0.0f));
			pVertices[5] = CTexturedVertex(XMFLOAT3(-fx, +fy, fz), XMFLOAT2(1.0f, 0.0f));
		}
	}

	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, sizeof(CTexturedVertex) * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = sizeof(CTexturedVertex);
	m_d3dPositionBufferView.SizeInBytes = sizeof(CTexturedVertex) * m_nVertices;
}

CTexturedRectMesh::~CTexturedRectMesh()
{
}

void CTexturedRectMesh::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(VS_CB_MESH_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbMesh = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbMesh->Map(0, NULL, (void**)&m_pcbMappedMesh);
}

void CTexturedRectMesh::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pcbMappedMesh)
	{
		::memcpy(&m_pcbMappedMesh->m_bIsClicked, &m_bIsClicked, sizeof(bool));
		::memcpy(&m_pcbMappedMesh->type, &m_nType, sizeof(int));
		::memcpy(&m_pcbMappedMesh->value, &m_fTextureValue, sizeof(float));
		::memcpy(&m_pcbMappedMesh->uvX, &m_fTexutreUV.x, sizeof(float));
		::memcpy(&m_pcbMappedMesh->uvY, &m_fTexutreUV.y, sizeof(float));
		/*if (m_fTexutreUV.x > 0.f && m_fTexutreUV.y > 0.f)
			cout << "My Type is " << m_nType << ", " << m_fTexutreUV.x << ", " << m_fTexutreUV.y << endl;*/
		D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = m_pd3dcbMesh->GetGPUVirtualAddress();
		pd3dCommandList->SetGraphicsRootConstantBufferView(19, d3dGpuVirtualAddress);
	}
}

void CTexturedRectMesh::ReleaseShaderVariables()
{
	if (m_pd3dcbMesh)
	{
		m_pd3dcbMesh->Unmap(0, NULL);
		m_pd3dcbMesh->Release();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CBoundingBoxTexturedMesh::CBoundingBoxTexturedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fWidth, float fHeight, float fDepth) : CMesh(pd3dDevice, pd3dCommandList)
{
	m_nVertices = 36;
	m_nStride = sizeof(CIlluminatedTexturedVertex);
	m_nOffset = 0;
	m_nSlot = 0;
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	float fx = fWidth * 0.5f, fy = fHeight * 0.5f, fz = fDepth * 0.5f;

	XMFLOAT3 pxmf3Positions[36];
	int i = 0;
	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, -fz); //3
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, -fz); //4
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, -fz); //8

	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, -fz); //3
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, -fz); //8
	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, -fz); //7

	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, +fz); //1
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, +fz); //2
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, -fz); //4

	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, +fz); //1
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, -fz); //4
	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, -fz); //3

	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, +fz); //5
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, +fz); //6
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, +fz); //2

	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, +fz); //5
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, +fz); //2
	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, +fz); //1

	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, -fz); //7
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, -fz); //8
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, +fz); //6

	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, -fz); //7
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, +fz); //6
	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, +fz); //5

	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, +fz); //1
	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, -fz); //3
	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, -fz); //7

	pxmf3Positions[i++] = XMFLOAT3(-fx, +fy, +fz); //1
	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, -fz); //7
	pxmf3Positions[i++] = XMFLOAT3(-fx, -fy, +fz); //5

	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, -fz); //4
	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, +fz); //2
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, +fz); //6

	pxmf3Positions[i++] = XMFLOAT3(+fx, +fy, -fz); //4
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, +fz); //6
	pxmf3Positions[i++] = XMFLOAT3(+fx, -fy, -fz); //8

	XMFLOAT2 pxmf2TexCoords[36];
	i = 0;
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);

	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);

	XMFLOAT3 pxmf3Normals[36];
	CalculateVertexNormals(pxmf3Normals, pxmf3Positions, m_nVertices, NULL, 0);

	CIlluminatedTexturedVertex pVertices[36];
	for (int i = 0; i < 36; i++) pVertices[i] = CIlluminatedTexturedVertex(pxmf3Positions[i], pxmf3Normals[i], pxmf2TexCoords[i]);

	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = m_nStride;
	m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
}

//CBoundingBoxTexturedMesh::CBoundingBoxTexturedMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, std::vector<XMFLOAT3> BBPostion, int bbCount) : CMesh(pd3dDevice, pd3dCommandList)
//{
//	m_nVertices = 36;
//	m_nStride = sizeof(CIlluminatedTexturedVertex);
//	m_nOffset = 0;
//	m_nSlot = 0;
//	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
//	XMFLOAT3 bbPos[8];
//	for (int i = 0; i < 8; ++i)
//	{
//		bbPos[i] = BBPostion[8 * bbCount + i];
//	}
//
//	XMFLOAT3 pxmf3Positions[36];
//	int i = 0;
//	pxmf3Positions[i++] = bbPos[0]; //3
//	pxmf3Positions[i++] = bbPos[3]; //4
//	pxmf3Positions[i++] = bbPos[2]; //8
//
//	pxmf3Positions[i++] = bbPos[0]; //3
//	pxmf3Positions[i++] = bbPos[2]; //8
//	pxmf3Positions[i++] = bbPos[1]; //7
//
//	pxmf3Positions[i++] = bbPos[4]; //1
//	pxmf3Positions[i++] = bbPos[7]; //2
//	pxmf3Positions[i++] = bbPos[3]; //4
//
//	pxmf3Positions[i++] = bbPos[4]; //1
//	pxmf3Positions[i++] = bbPos[3]; //4
//	pxmf3Positions[i++] = bbPos[0]; //3
//
//	pxmf3Positions[i++] = bbPos[5]; //5
//	pxmf3Positions[i++] = bbPos[6]; //6
//	pxmf3Positions[i++] = bbPos[7]; //2
//
//	pxmf3Positions[i++] = bbPos[5]; //5
//	pxmf3Positions[i++] = bbPos[7]; //2
//	pxmf3Positions[i++] = bbPos[4]; //1
//
//	pxmf3Positions[i++] = bbPos[1]; //7
//	pxmf3Positions[i++] = bbPos[2]; //8
//	pxmf3Positions[i++] = bbPos[6]; //6
//
//	pxmf3Positions[i++] = bbPos[1]; //7
//	pxmf3Positions[i++] = bbPos[6]; //6
//	pxmf3Positions[i++] = bbPos[5]; //5
//
//	pxmf3Positions[i++] = bbPos[4]; //1
//	pxmf3Positions[i++] = bbPos[0]; //3
//	pxmf3Positions[i++] = bbPos[1]; //7
//
//	pxmf3Positions[i++] = bbPos[4]; //1
//	pxmf3Positions[i++] = bbPos[1]; //7
//	pxmf3Positions[i++] = bbPos[5]; //5
//
//	pxmf3Positions[i++] = bbPos[3]; //4
//	pxmf3Positions[i++] = bbPos[7]; //2
//	pxmf3Positions[i++] = bbPos[6]; //6
//
//	pxmf3Positions[i++] = bbPos[3]; //4
//	pxmf3Positions[i++] = bbPos[6]; //6
//	pxmf3Positions[i++] = bbPos[2]; //8
//
//	XMFLOAT2 pxmf2TexCoords[36];
//	i = 0;
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 0.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(1.0f, 1.0f);
//	pxmf2TexCoords[i++] = XMFLOAT2(0.0f, 1.0f);
//
//	XMFLOAT3 pxmf3Normals[36];
//	CalculateVertexNormals(pxmf3Normals, pxmf3Positions, m_nVertices, NULL, 0);
//
//	CIlluminatedTexturedVertex pVertices[36];
//	for (int i = 0; i < 36; i++) pVertices[i] = CIlluminatedTexturedVertex(pxmf3Positions[i], pxmf3Normals[i], pxmf2TexCoords[i]);
//
//	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);
//
//	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
//	m_d3dPositionBufferView.StrideInBytes = m_nStride;
//	m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
//}

CBoundingBoxTexturedMesh::~CBoundingBoxTexturedMesh()
{
}

void CBoundingBoxTexturedMesh::CalculateTriangleListVertexNormals(XMFLOAT3* pxmf3Normals, XMFLOAT3* pxmf3Positions, int nVertices)
{
	int nPrimitives = nVertices / 3;
	UINT nIndex0, nIndex1, nIndex2;
	for (int i = 0; i < nPrimitives; i++)
	{
		nIndex0 = i * 3 + 0;
		nIndex1 = i * 3 + 1;
		nIndex2 = i * 3 + 2;
		XMFLOAT3 xmf3Edge01 = Vector3::Subtract(pxmf3Positions[nIndex1], pxmf3Positions[nIndex0]);
		XMFLOAT3 xmf3Edge02 = Vector3::Subtract(pxmf3Positions[nIndex2], pxmf3Positions[nIndex0]);
		pxmf3Normals[nIndex0] = pxmf3Normals[nIndex1] = pxmf3Normals[nIndex2] = Vector3::CrossProduct(xmf3Edge01, xmf3Edge02, true);
	}
}

void CBoundingBoxTexturedMesh::CalculateTriangleListVertexNormals(XMFLOAT3* pxmf3Normals, XMFLOAT3* pxmf3Positions, UINT nVertices, UINT* pnIndices, UINT nIndices)
{
	UINT nPrimitives = (pnIndices) ? (nIndices / 3) : (nVertices / 3);
	XMFLOAT3 xmf3SumOfNormal, xmf3Edge01, xmf3Edge02, xmf3Normal;
	UINT nIndex0, nIndex1, nIndex2;
	for (UINT j = 0; j < nVertices; j++)
	{
		xmf3SumOfNormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		for (UINT i = 0; i < nPrimitives; i++)
		{
			nIndex0 = pnIndices[i * 3 + 0];
			nIndex1 = pnIndices[i * 3 + 1];
			nIndex2 = pnIndices[i * 3 + 2];
			if (pnIndices && ((nIndex0 == j) || (nIndex1 == j) || (nIndex2 == j)))
			{
				xmf3Edge01 = Vector3::Subtract(pxmf3Positions[nIndex1], pxmf3Positions[nIndex0]);
				xmf3Edge02 = Vector3::Subtract(pxmf3Positions[nIndex2], pxmf3Positions[nIndex0]);
				xmf3Normal = Vector3::CrossProduct(xmf3Edge01, xmf3Edge02, false);
				xmf3SumOfNormal = Vector3::Add(xmf3SumOfNormal, xmf3Normal);
			}
		}
		pxmf3Normals[j] = Vector3::Normalize(xmf3SumOfNormal);
	}
}

void CBoundingBoxTexturedMesh::CalculateTriangleStripVertexNormals(XMFLOAT3* pxmf3Normals, XMFLOAT3* pxmf3Positions, UINT nVertices, UINT* pnIndices, UINT nIndices)
{
	UINT nPrimitives = (pnIndices) ? (nIndices - 2) : (nVertices - 2);
	XMFLOAT3 xmf3SumOfNormal(0.0f, 0.0f, 0.0f);
	UINT nIndex0, nIndex1, nIndex2;
	for (UINT j = 0; j < nVertices; j++)
	{
		xmf3SumOfNormal = XMFLOAT3(0.0f, 0.0f, 0.0f);
		for (UINT i = 0; i < nPrimitives; i++)
		{
			nIndex0 = ((i % 2) == 0) ? (i + 0) : (i + 1);
			if (pnIndices) nIndex0 = pnIndices[nIndex0];
			nIndex1 = ((i % 2) == 0) ? (i + 1) : (i + 0);
			if (pnIndices) nIndex1 = pnIndices[nIndex1];
			nIndex2 = (pnIndices) ? pnIndices[i + 2] : (i + 2);
			if ((nIndex0 == j) || (nIndex1 == j) || (nIndex2 == j))
			{
				XMFLOAT3 xmf3Edge01 = Vector3::Subtract(pxmf3Positions[nIndex1], pxmf3Positions[nIndex0]);
				XMFLOAT3 xmf3Edge02 = Vector3::Subtract(pxmf3Positions[nIndex2], pxmf3Positions[nIndex0]);
				XMFLOAT3 xmf3Normal = Vector3::CrossProduct(xmf3Edge01, xmf3Edge02, true);
				xmf3SumOfNormal = Vector3::Add(xmf3SumOfNormal, xmf3Normal);
			}
		}
		pxmf3Normals[j] = Vector3::Normalize(xmf3SumOfNormal);
	}
}

void CBoundingBoxTexturedMesh::CalculateVertexNormals(XMFLOAT3* pxmf3Normals, XMFLOAT3* pxmf3Positions, int nVertices, UINT* pnIndices, int nIndices)
{
	switch (m_d3dPrimitiveTopology)
	{
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST:
		if (pnIndices)
			CalculateTriangleListVertexNormals(pxmf3Normals, pxmf3Positions, nVertices, pnIndices, nIndices);
		else
			CalculateTriangleListVertexNormals(pxmf3Normals, pxmf3Positions, nVertices);
		break;
	case D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:
		CalculateTriangleStripVertexNormals(pxmf3Normals, pxmf3Positions, nVertices, pnIndices, nIndices);
		break;
	default:
		break;
	}
}

/// <summary>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>


CParticleMesh::CParticleMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Velocity, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles, UINT nType) : CMesh(pd3dDevice, pd3dCommandList)
{
	CreateVertexBuffer(pd3dDevice, pd3dCommandList, xmf3Position, xmf3Velocity, fLifetime, xmf3Acceleration, xmf3Color, xmf2Size, nType);
	CreateStreamOutputBuffer(pd3dDevice, pd3dCommandList, nMaxParticles);
	ClientMemoryProfileRecordParticle(pd3dDevice, m_pd3dStreamOutputBuffer, m_pd3dDrawBuffer, nMaxParticles, m_nStride);
}

CParticleMesh::~CParticleMesh()
{
	if (m_pd3dStreamOutputBuffer) m_pd3dStreamOutputBuffer->Release();
	if (m_pd3dDrawBuffer) m_pd3dDrawBuffer->Release();
	if (m_pd3dDefaultBufferFilledSize) m_pd3dDefaultBufferFilledSize->Release();
	if (m_pd3dUploadBufferFilledSize) m_pd3dUploadBufferFilledSize->Release();

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
	if (m_pd3dSOQueryBuffer) m_pd3dSOQueryBuffer->Release();
	if (m_pd3dSOQueryHeap) m_pd3dSOQueryHeap->Release();
#else
	if (m_pd3dReadBackBufferFilledSize) m_pd3dReadBackBufferFilledSize->Release();
#endif
}

void CParticleMesh::CreateVertexBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Velocity, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nType)
{
	m_nVertices = 1;
	m_nStride = sizeof(CParticleVertex);
	m_d3dPrimitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;

	CParticleVertex pVertices[1];

	//pVertices[0].m_xmf3Position = xmf3Position;
	pVertices[0].m_xmf3Position = XMFLOAT3(0,0,0);
	pVertices[0].m_xmf3Velocity = xmf3Velocity;
	pVertices[0].m_fLifetime = fLifetime;
	pVertices[0].m_nType = nType;

	m_pd3dPositionBuffer = CreateBufferResource(pd3dDevice, pd3dCommandList, pVertices, m_nStride * m_nVertices, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, &m_pd3dPositionUploadBuffer);

	m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
	m_d3dPositionBufferView.StrideInBytes = m_nStride;
	m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
}

void CParticleMesh::CreateStreamOutputBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nMaxParticles)
{
	m_nMaxParticles = nMaxParticles;

	m_pd3dStreamOutputBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, (m_nStride * m_nMaxParticles), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, NULL);
	m_pd3dDrawBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, (m_nStride * m_nMaxParticles), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	UINT64 nBufferFilledSize = 0;
	m_pd3dDefaultBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, &nBufferFilledSize, sizeof(UINT64), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, NULL);

	m_pd3dUploadBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(UINT64), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, NULL);
	m_pd3dUploadBufferFilledSize->Map(0, NULL, (void**)&m_pnUploadBufferFilledSize);

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
	D3D12_QUERY_HEAP_DESC d3dQueryHeapDesc = { };
	d3dQueryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_SO_STATISTICS;
	d3dQueryHeapDesc.Count = 1;
	d3dQueryHeapDesc.NodeMask = 0;
	pd3dDevice->CreateQueryHeap(&d3dQueryHeapDesc, __uuidof(ID3D12QueryHeap), (void**)&m_pd3dSOQueryHeap);

	m_pd3dSOQueryBuffer = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(D3D12_QUERY_DATA_SO_STATISTICS), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, NULL);
#else
	m_pd3dReadBackBufferFilledSize = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, sizeof(UINT64), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, NULL);
#endif
}

void CParticleMesh::PreRender(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
	if (nPipelineState == 0)
	{
		if (m_bStart)
		{
			m_bStart = false;

			m_nVertices = 1;

			//m_d3dVertexBufferView.BufferLocation = m_pd3dVertexBuffer->GetGPUVirtualAddress();
			//m_d3dVertexBufferView.StrideInBytes = m_nStride;
			//m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

			m_d3dPositionBufferView.BufferLocation = m_pd3dPositionBuffer->GetGPUVirtualAddress();
			m_d3dPositionBufferView.StrideInBytes = m_nStride;
			m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
		}
		else
		{
			//m_d3dVertexBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
			//m_d3dVertexBufferView.StrideInBytes = m_nStride;
			//m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

			m_d3dPositionBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
			m_d3dPositionBufferView.StrideInBytes = m_nStride;
			m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
		}
		m_d3dStreamOutputBufferView.BufferLocation = m_pd3dStreamOutputBuffer->GetGPUVirtualAddress();
		m_d3dStreamOutputBufferView.SizeInBytes = m_nStride * m_nMaxParticles;
		m_d3dStreamOutputBufferView.BufferFilledSizeLocation = m_pd3dDefaultBufferFilledSize->GetGPUVirtualAddress();

		//*m_pnUploadBufferFilledSize = m_nStride * m_nVertices;
		*m_pnUploadBufferFilledSize = 0;

		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize, D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);
		pd3dCommandList->CopyResource(m_pd3dDefaultBufferFilledSize, m_pd3dUploadBufferFilledSize);
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);
	}
	else if (nPipelineState == 1)
	{
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dStreamOutputBuffer, D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDrawBuffer, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT);

		::SwapResourcePointer(&m_pd3dDrawBuffer, &m_pd3dStreamOutputBuffer);

		//m_d3dVertexBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
		//m_d3dVertexBufferView.StrideInBytes = m_nStride;
		//m_d3dVertexBufferView.SizeInBytes = m_nStride * m_nVertices;

		m_d3dPositionBufferView.BufferLocation = m_pd3dDrawBuffer->GetGPUVirtualAddress();
		m_d3dPositionBufferView.StrideInBytes = m_nStride;
		m_d3dPositionBufferView.SizeInBytes = m_nStride * m_nVertices;
	}
}

void CParticleMesh::Render(ID3D12GraphicsCommandList* pd3dCommandList, int nPipelineState)
{
	if (nPipelineState == 0)
	{
		D3D12_STREAM_OUTPUT_BUFFER_VIEW pStreamOutputBufferViews[1] = { m_d3dStreamOutputBufferView };
		pd3dCommandList->SOSetTargets(0, 1, pStreamOutputBufferViews);

#ifdef _WITH_QUERY_DATA_SO_STATISTICS
		pd3dCommandList->BeginQuery(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0);
		CMesh::Render(pd3dCommandList);
		pd3dCommandList->EndQuery(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0);

		pd3dCommandList->ResolveQueryData(m_pd3dSOQueryHeap, D3D12_QUERY_TYPE_SO_STATISTICS_STREAM0, 0, 1, m_pd3dSOQueryBuffer, 0);
#else
		CMesh::Render(pd3dCommandList); //Stream Output to m_pd3dStreamOutputBuffer
		//CMesh::Render(pd3dCommandList, 0); //Stream Output to m_pd3dStreamOutputBuffer

		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize, D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pd3dCommandList->CopyResource(m_pd3dReadBackBufferFilledSize, m_pd3dDefaultBufferFilledSize);
		::SynchronizeResourceTransition(pd3dCommandList, m_pd3dDefaultBufferFilledSize, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);
#endif
	}
	else if (nPipelineState == 1)
	{
		pd3dCommandList->SOSetTargets(0, 1, NULL);

		CMesh::Render(pd3dCommandList); //Render m_pd3dDrawBuffer 
		//CMesh::Render(pd3dCommandList,0); //Render m_pd3dDrawBuffer 
	}
}

//#define _WITH_DEBUG_STREAM_OUTPUT_VERTICES

void CParticleMesh::ParticlePostRender(int nPipelineState)
{
	if (nPipelineState == 0)
	{
#ifdef _WITH_QUERY_DATA_SO_STATISTICS
		D3D12_RANGE d3dReadRange = { 0, 0 };
		UINT8* pBufferDataBegin = NULL;
		m_pd3dSOQueryBuffer->Map(0, &d3dReadRange, (void**)&m_pd3dSOQueryDataStatistics);
		if (m_pd3dSOQueryDataStatistics) m_nVertices = (UINT)m_pd3dSOQueryDataStatistics->NumPrimitivesWritten;
		m_pd3dSOQueryBuffer->Unmap(0, NULL);
#else
		UINT64* pnReadBackBufferFilledSize = NULL;
		m_pd3dReadBackBufferFilledSize->Map(0, NULL, (void**)&pnReadBackBufferFilledSize);
		if (pnReadBackBufferFilledSize)
			m_nVertices = UINT(*pnReadBackBufferFilledSize) / m_nStride;
		else
			cout << "pnReadBackBufferFilledSize is null" << endl;
		m_pd3dReadBackBufferFilledSize->Unmap(0, NULL);
#endif

		//::gnCurrentParticles = m_nVertices;
#ifdef _WITH_DEBUG_STREAM_OUTPUT_VERTICES
		TCHAR pstrDebug[256] = { 0 };
		//cout << m_nVertices << endl;
		_stprintf_s(pstrDebug, 256, _T("Stream Output Vertices = %d\n"), m_nVertices);
		OutputDebugString(pstrDebug);
#endif
		if ((m_nVertices == 0) || (m_nVertices >= MAX_PARTICLES)) m_bStart = true;
	}
}
