//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"
#include "Shader.h"
#include "Scene.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Frustum.h"
#include "NameSpace.h"
#include "SoundManager.h"




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CTexture::CTexture(int nTextures, UINT nTextureType, int nSamplers, int nRootParameters)
{
	m_nTextureType = nTextureType;

	m_nTextures = nTextures;
	if (m_nTextures > 0)
	{
		m_ppd3dTextureUploadBuffers = new ID3D12Resource * [m_nTextures];
		m_ppd3dTextures = new ID3D12Resource * [m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_ppd3dTextureUploadBuffers[i] = m_ppd3dTextures[i] = NULL;

		m_sharepd3dSrvGpuDescriptorHandles = make_shared<D3D12_GPU_DESCRIPTOR_HANDLE[]>(m_nTextures);

		m_pnResourceTypes = new UINT[m_nTextures];
		m_pdxgiBufferFormats = new DXGI_FORMAT[m_nTextures];
		m_pnBufferElements = new int[m_nTextures];
		m_pnBufferStrides = new int[m_nTextures];
	}
	m_nRootParameters = nRootParameters;
	if (nRootParameters > 0) m_sharepnRootParameterIndices = make_shared<UINT[]>(nRootParameters);

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0) m_pd3dSamplerGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nSamplers];
}

CTexture::CTexture(const CTexture& rhs) : m_nTextureType{ rhs.m_nTextureType }, m_nTextures{ rhs.m_nTextures }, m_nRootParameters{rhs.m_nRootParameters}, m_nSamplers{rhs.m_nSamplers},
m_nReferences{rhs.m_nReferences}
{
	if (m_nTextures > 0)
	{
		m_ppd3dTextureUploadBuffers = new ID3D12Resource * [m_nTextures];
		m_ppd3dTextures = new ID3D12Resource * [m_nTextures];
		m_pnResourceTypes = new UINT[m_nTextures];
		m_pdxgiBufferFormats = new DXGI_FORMAT[m_nTextures];
		m_pnBufferElements = new int[m_nTextures];
		m_pnBufferStrides = new int[m_nTextures];
		m_sharepd3dSrvGpuDescriptorHandles = rhs.m_sharepd3dSrvGpuDescriptorHandles;
	}
	if (m_nRootParameters > 0) 
		m_sharepnRootParameterIndices = rhs.m_sharepnRootParameterIndices;
	if (m_nSamplers > 0) m_pd3dSamplerGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nSamplers];

	for (int i = 0; i < m_nTextures; ++i)
	{
		m_ppd3dTextures[i] = rhs.m_ppd3dTextures[i];
		m_ppd3dTextures[i]->AddRef();
		m_ppd3dTextureUploadBuffers[i] = rhs.m_ppd3dTextureUploadBuffers[i];
		m_pnResourceTypes[i] = rhs.m_pnResourceTypes[i];
		m_pdxgiBufferFormats[i] = rhs.m_pdxgiBufferFormats[i];
		m_pnBufferElements[i] = rhs.m_pnBufferElements[i];
		m_pnBufferStrides[i] = rhs.m_pnBufferStrides[i];
	}

	for (int i = 0; i < m_nSamplers; ++i)
	{
		m_pd3dSamplerGpuDescriptorHandles[i] = rhs.m_pd3dSamplerGpuDescriptorHandles[i];
	}
}

CTexture::~CTexture()
{
	if (m_ppd3dTextures)
	{
		for (int i = 0; i < m_nTextures; i++) 
			if (m_ppd3dTextures[i])
			{
				if (!m_ppd3dTextures[i]->Release()) {
					m_ppd3dTextures[i] = nullptr;
				}
			}		
		delete[] m_ppd3dTextures;
		m_ppd3dTextures = nullptr;
	}
	if (m_pnResourceTypes) delete[] m_pnResourceTypes;
	if (m_pdxgiBufferFormats) delete[] m_pdxgiBufferFormats;
	if (m_pnBufferElements) delete[] m_pnBufferElements;
	if (m_pnBufferStrides) delete[] m_pnBufferStrides;

	if (m_pd3dSamplerGpuDescriptorHandles) delete[] m_pd3dSamplerGpuDescriptorHandles;
}

void CTexture::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex)
{
	m_sharepnRootParameterIndices.get()[nIndex] = nRootParameterIndex;
}

void CTexture::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle)
{
	m_sharepd3dSrvGpuDescriptorHandles.get()[nIndex] = d3dSrvGpuDescriptorHandle;
}

void CTexture::SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle)
{
	m_pd3dSamplerGpuDescriptorHandles[nIndex] = d3dSamplerGpuDescriptorHandle;
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_nTextures)
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			pd3dCommandList->SetGraphicsRootDescriptorTable(/*m_pnRootParameterIndices*/m_sharepnRootParameterIndices.get()[i], m_sharepd3dSrvGpuDescriptorHandles.get()[i]/*m_pd3dSrvGpuDescriptorHandles[i]*/);
		}
	}
	else
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(m_sharepnRootParameterIndices.get()[0]/*m_pnRootParameterIndices[0]*/, m_sharepd3dSrvGpuDescriptorHandles.get()[0]/*m_pd3dSrvGpuDescriptorHandles[0]*/);
	}
}

void CTexture::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
{
	pd3dCommandList->SetGraphicsRootDescriptorTable(m_sharepnRootParameterIndices.get()[nParameterIndex]/*m_pnRootParameterIndices[nParameterIndex]*/, m_sharepd3dSrvGpuDescriptorHandles.get()[nTextureIndex]/*m_pd3dSrvGpuDescriptorHandles[nTextureIndex]*/);
}

void CTexture::ReleaseShaderVariables()
{
}

void CTexture::ReleaseUploadBuffers()
{
	if (m_ppd3dTextureUploadBuffers)
	{
		for (int i = 0; i < m_nTextures; i++) 
			if (m_ppd3dTextureUploadBuffers[i]) {
				m_ppd3dTextureUploadBuffers[i]->Release();
			}
		delete[] m_ppd3dTextureUploadBuffers;
		m_ppd3dTextureUploadBuffers = NULL;
	}
}

void CTexture::LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &m_ppd3dTextureUploadBuffers[nIndex], D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void CTexture::LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = RESOURCE_BUFFER;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pnBufferElements[nIndex] = nElements;
	m_ppd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, &m_ppd3dTextureUploadBuffers[nIndex]);
}

void CTexture::CreateBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = RESOURCE_BUFFER;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pnBufferElements[nIndex] = nElements;
	m_pnBufferStrides[nIndex] = nStride;
	m_ppd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, d3dHeapType, d3dResourceStates, &m_ppd3dTextureUploadBuffers[nIndex]);
}

ID3D12Resource* CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTexture2DResource(pd3dDevice, pd3dCommandList, nWidth, nHeight, nElements, nMipLevels, dxgiFormat, d3dResourceFlags, d3dResourceStates, pd3dClearValue);
	return(m_ppd3dTextures[nIndex]);
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType(nIndex);
	switch (nTextureType)
	{
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		if (d3dResourceDesc.Format == DXGI_FORMAT_D32_FLOAT) d3dShaderResourceViewDesc.Format = DXGI_FORMAT_R32_FLOAT;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_pnBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CMaterial::CMaterial(int nTextures)
{
	m_nTextures = nTextures;

	m_ppTextures = new CTexture*[m_nTextures];
	m_ppstrTextureNames = new _TCHAR[m_nTextures][64];
	for (int i = 0; i < m_nTextures; i++) m_ppTextures[i] = NULL;
	for (int i = 0; i < m_nTextures; i++) m_ppstrTextureNames[i][0] = '\0';
}

CMaterial::~CMaterial()
{
	if (m_pShader) m_pShader->Release();

	if (m_nTextures > 0)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppTextures[i]) m_ppTextures[i]->Release();
		delete[] m_ppTextures;

		if (m_ppstrTextureNames) delete[] m_ppstrTextureNames;
	}

	ReleaseShaderVariables();
}

void CMaterial::SetShader(CShader *pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CMaterial::SetTexture(CTexture *pTexture, UINT nTexture) 
{ 
	if (m_ppTextures[nTexture]) m_ppTextures[nTexture]->Release();
	m_ppTextures[nTexture] = pTexture; 
	if (m_ppTextures[nTexture]) m_ppTextures[nTexture]->AddRef();  
}

void CMaterial::ReleaseUploadBuffers()
{
	for (int i = 0; i < m_nTextures; i++)
	{
		if (m_ppTextures[i]) {
			m_ppTextures[i]->ReleaseUploadBuffers();
		}
	}
}

void CMaterial::ReleaseShaderVariables()
{
	if (m_pd3dcbMaterial)
	{
		m_pd3dcbMaterial->Unmap(0, NULL);
		m_pd3dcbMaterial->Release();
	}
}

CShader *CMaterial::m_pSkinnedAnimationShader = NULL;
CShader *CMaterial::m_pStandardShader = NULL;

void CMaterial::PrepareShaders(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature)
{
	if (!m_pStandardShader) {
		m_pStandardShader = new CStandardShader();
		m_pStandardShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
		m_pStandardShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_pStandardShader->AddRef();
	}

	if (!m_pSkinnedAnimationShader) {
		m_pSkinnedAnimationShader = new CSkinnedAnimationStandardShader();
		m_pSkinnedAnimationShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
		m_pSkinnedAnimationShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_pSkinnedAnimationShader->AddRef();
	}
}

void CMaterial::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList)
{
	/*pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_xmf4AmbientColor, 16);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_xmf4AlbedoColor, 20);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_xmf4SpecularColor, 24);
	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 4, &m_xmf4EmissiveColor, 28);

	pd3dCommandList->SetGraphicsRoot32BitConstants(1, 1, &m_nType, 32);*/

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbMaterialGpuVirtualAddress = m_pd3dcbMaterial->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(24, d3dcbMaterialGpuVirtualAddress); 
	

	::memcpy(&m_pcbMappedMaterial->m_xmf4AmbientColor, &m_xmf4AmbientColor, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedMaterial->m_xmf4AlbedoColor, &m_xmf4AlbedoColor, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedMaterial->m_xmf4SpecularColor, &m_xmf4SpecularColor, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedMaterial->m_xmf4EmissiveColor, &m_xmf4EmissiveColor, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedMaterial->m_nType, &m_nType, sizeof(UINT));

	for (int i = 0; i < m_nTextures; i++)
	{
		if (m_ppTextures[i]) m_ppTextures[i]->UpdateShaderVariables(pd3dCommandList);
		//		if (m_ppTextures[i]) m_ppTextures[i]->UpdateShaderVariable(pd3dCommandList, 0, 0);
	}


}

void CMaterial::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(MATERIAL_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbMaterial = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbMaterial->Map(0, NULL, (void**)&m_pcbMappedMaterial);
}

void CMaterial::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nType, UINT nRootParameter, _TCHAR* pwstrTextureName, CTexture** ppTexture, CGameObject* pParent, FILE* pInFile, CShader* pShader)
{
	char pstrTextureName[64] = { '\0' };

	BYTE nStrLength = 64;
	UINT nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
	nReads = (UINT)::fread(pstrTextureName, sizeof(char), nStrLength, pInFile);
	pstrTextureName[nStrLength] = '\0';

	bool bDuplicated = false;
	if (strcmp(pstrTextureName, "null"))
	{
		SetMaterialType(nType);

		char pstrFilePath[64] = { '\0' };
		strcpy_s(pstrFilePath, 64, "Model/Textures/");

		bDuplicated = (pstrTextureName[0] == '@');
		strcpy_s(pstrFilePath + 15, 64 - 15, (bDuplicated) ? (pstrTextureName + 1) : pstrTextureName);
		strcpy_s(pstrFilePath + 15 + ((bDuplicated) ? (nStrLength - 1) : nStrLength), 64 - 15 - ((bDuplicated) ? (nStrLength - 1) : nStrLength), ".dds");

		size_t nConverted = 0;
		mbstowcs_s(&nConverted, pwstrTextureName, 64, pstrFilePath, _TRUNCATE);

		//#define _WITH_DISPLAY_TEXTURE_NAME

#ifdef _WITH_DISPLAY_TEXTURE_NAME
		static int nTextures = 0, nRepeatedTextures = 0;
		TCHAR pstrDebug[256] = { 0 };
		_stprintf_s(pstrDebug, 256, _T("Texture Name: %d %c %s\n"), (pstrTextureName[0] == '@') ? nRepeatedTextures++ : nTextures++, (pstrTextureName[0] == '@') ? '@' : ' ', pwstrTextureName);
		OutputDebugString(pstrDebug);
#endif
		if (!bDuplicated)
		{
			*ppTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
			(*ppTexture)->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, pwstrTextureName, RESOURCE_TEXTURE2D, 0);
			if (*ppTexture) (*ppTexture)->AddRef();

			CScene::CreateShaderResourceViews(pd3dDevice, *ppTexture, 0, nRootParameter);
		}
		else
		{
			if (pParent)
			{
				while (pParent)
				{
					if (!pParent->m_pParent) break;
					pParent = pParent->m_pParent;
				}
				CGameObject* pRootGameObject = pParent;
				*ppTexture = pRootGameObject->FindReplicatedTexture(pwstrTextureName);
				if (*ppTexture) (*ppTexture)->AddRef();
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationSet::CAnimationSet(float fLength, int nFramesPerSecond, int nKeyFrames, int nAnimatedBones, char *pstrName)
{
	m_fLength = fLength;
	m_nFramesPerSecond = nFramesPerSecond;
	m_nKeyFrames = nKeyFrames;

	strcpy_s(m_pstrAnimationSetName, 64, pstrName);

#ifdef _WITH_ANIMATION_SRT
	m_nKeyFrameTranslations = nKeyFrames;
	m_pfKeyFrameTranslationTimes = new float[m_nKeyFrameTranslations];
	m_ppxmf3KeyFrameTranslations = new XMFLOAT3 * [m_nKeyFrameTranslations];
	for (int i = 0; i < m_nKeyFrameTranslations; i++) m_ppxmf3KeyFrameTranslations[i] = new XMFLOAT4X4[nAnimatedBones];

	m_nKeyFrameScales = nKeyFrames;
	m_pfKeyFrameScaleTimes = new float[m_nKeyFrameScales];
	m_ppxmf3KeyFrameScales = new XMFLOAT3 * [m_nKeyFrameScales];
	for (int i = 0; i < m_nKeyFrameScales; i++) m_ppxmf3KeyFrameScales[i] = new XMFLOAT4X4[nAnimatedBones];

	m_nKeyFrameRotations = nKeyFrames;
	m_pfKeyFrameRotationTimes = new float[m_nKeyFrameRotations];
	m_ppxmf4KeyFrameRotations = new XMFLOAT3 * [m_nKeyFrameRotations];
	for (int i = 0; i < m_nKeyFrameRotations; i++) m_ppxmf4KeyFrameRotations[i] = new XMFLOAT4X4[nAnimatedBones];
#else
	m_pfKeyFrameTimes = new float[nKeyFrames];
	m_ppxmf4x4KeyFrameTransforms = new XMFLOAT4X4*[nKeyFrames];
	for (int i = 0; i < nKeyFrames; i++) m_ppxmf4x4KeyFrameTransforms[i] = new XMFLOAT4X4[nAnimatedBones];
#endif
}

CAnimationSet::~CAnimationSet()
{
#ifdef _WITH_ANIMATION_SRT
	if (m_pfKeyFrameTranslationTimes) delete[] m_pfKeyFrameTranslationTimes;
	for (int j = 0; j < m_nKeyFrameTranslations; j++) if (m_ppxmf3KeyFrameTranslations[j]) delete[] m_ppxmf3KeyFrameTranslations[j];
	if (m_ppxmf3KeyFrameTranslations) delete[] m_ppxmf3KeyFrameTranslations;

	if (m_pfKeyFrameScaleTimes) delete[] m_pfKeyFrameScaleTimes;
	for (int j = 0; j < m_nKeyFrameScales; j++) if (m_ppxmf3KeyFrameScales[j]) delete[] m_ppxmf3KeyFrameScales[j];
	if (m_ppxmf3KeyFrameScales) delete[] m_ppxmf3KeyFrameScales;

	if (m_pfKeyFrameRotationTimes) delete[] m_pfKeyFrameRotationTimes;
	for (int j = 0; j < m_nKeyFrameRotations; j++) if (m_ppxmf4KeyFrameRotations[j]) delete[] m_ppxmf4KeyFrameRotations[j];
	if (m_ppxmf4KeyFrameRotations) delete[] m_ppxmf4KeyFrameRotations;
#else
	if (m_pfKeyFrameTimes) {
		delete[] m_pfKeyFrameTimes;
		m_pfKeyFrameTimes = nullptr;
	}
	for (int j = 0; j < m_nKeyFrames; j++) 
		if (m_ppxmf4x4KeyFrameTransforms[j]) {
			delete[] m_ppxmf4x4KeyFrameTransforms[j];
			m_ppxmf4x4KeyFrameTransforms[j] = nullptr;
		}
	if (m_ppxmf4x4KeyFrameTransforms) {
		delete[] m_ppxmf4x4KeyFrameTransforms;
		m_ppxmf4x4KeyFrameTransforms = nullptr;
	}
#endif
}

XMFLOAT4X4 CAnimationSet::GetSRT(int nBone, float fPosition)
{
	XMFLOAT4X4 xmf4x4Transform = Matrix4x4::Identity();
#ifdef _WITH_ANIMATION_SRT
	XMVECTOR S, R, T;
	for (int i = 0; i < (m_nKeyFrameTranslations - 1); i++)
	{
		if ((m_pfKeyFrameTranslationTimes[i] <= fPosition) && (fPosition <= m_pfKeyFrameTranslationTimes[i+1]))
		{
			float t = (fPosition - m_pfKeyFrameTranslationTimes[i]) / (m_pfKeyFrameTranslationTimes[i+1] - m_pfKeyFrameTranslationTimes[i]);
			T = XMVectorLerp(XMLoadFloat3(&m_ppxmf3KeyFrameTranslations[i][nBone]), XMLoadFloat3(&m_ppxmf3KeyFrameTranslations[i+1][nBone]), t);
			break;
		}
	}
	for (UINT i = 0; i < (m_nKeyFrameScales - 1); i++)
	{
		if ((m_pfKeyFrameScaleTimes[i] <= fPosition) && (fPosition <= m_pfKeyFrameScaleTimes[i+1]))
		{
			float t = (fPosition - m_pfKeyFrameScaleTimes[i]) / (m_pfKeyFrameScaleTimes[i+1] - m_pfKeyFrameScaleTimes[i]);
			S = XMVectorLerp(XMLoadFloat3(&m_ppxmf3KeyFrameScales[i][nBone]), XMLoadFloat3(&m_ppxmf3KeyFrameScales[i+1][nBone]), t);
			break;
		}
	}
	for (UINT i = 0; i < (m_nKeyFrameRotations - 1); i++)
	{
		if ((m_pfKeyFrameRotationTimes[i] <= fPosition) && (fPosition <= m_pfKeyFrameRotationTimes[i+1]))
		{
			float t = (m_fPosition - m_pfKeyFrameRotationTimes[i]) / (m_pfKeyFrameRotationTimes[i+1] - m_pfKeyFrameRotationTimes[i]);
			R = XMQuaternionSlerp(XMQuaternionConjugate(XMLoadFloat4(&m_ppxmf4KeyFrameRotations[i][nBone])), XMQuaternionConjugate(XMLoadFloat4(&m_ppxmf4KeyFrameRotations[i+1][nBone])), t);
			break;
		}
	}

	XMStoreFloat4x4(&xmf4x4Transform, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
#else   
	for (int i = 0; i < (m_nKeyFrames - 1); i++) 
	{
		if ((m_pfKeyFrameTimes[i] <= fPosition) && (fPosition < m_pfKeyFrameTimes[i+1]))
		{
			float t = (fPosition - m_pfKeyFrameTimes[i]) / (m_pfKeyFrameTimes[i+1] - m_pfKeyFrameTimes[i]);
			xmf4x4Transform = Matrix4x4::Interpolate(m_ppxmf4x4KeyFrameTransforms[i][nBone], m_ppxmf4x4KeyFrameTransforms[i+1][nBone], t);
			break;
		}
	}
	if (fPosition >= m_pfKeyFrameTimes[m_nKeyFrames-1]) xmf4x4Transform = m_ppxmf4x4KeyFrameTransforms[m_nKeyFrames-1][nBone];

#endif
	return(xmf4x4Transform);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationSets::CAnimationSets(int nAnimationSets)
{
	m_nAnimationSets = nAnimationSets;
	m_pAnimationSets = new CAnimationSet*[nAnimationSets];
}

CAnimationSets::~CAnimationSets()
{
	for (int i = 0; i < m_nAnimationSets; i++)
		if (m_pAnimationSets[i])
		{
			delete m_pAnimationSets[i];
			m_pAnimationSets[i] = nullptr;
		}
	if (m_pAnimationSets) {
		delete[] m_pAnimationSets;
		m_pAnimationSets = nullptr;
	}

	if (m_ppAnimatedBoneFrameCaches) delete[] m_ppAnimatedBoneFrameCaches;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationTrack::~CAnimationTrack()
{ 
	if (m_pCallbackKeys) delete[] m_pCallbackKeys;
	if (m_pAnimationCallbackHandler) delete m_pAnimationCallbackHandler;
}

void CAnimationTrack::SetCallbackKeys(int nCallbackKeys)
{
	m_nCallbackKeys = nCallbackKeys;
	m_pCallbackKeys = new CALLBACKKEY[nCallbackKeys];
}

void CAnimationTrack::SetCallbackKey(int nKeyIndex, float fKeyTime, void* pData)
{
	m_pCallbackKeys[nKeyIndex].m_fTime = fKeyTime;
	m_pCallbackKeys[nKeyIndex].m_pCallbackData = pData;
}

void CAnimationTrack::SetAnimationCallbackHandler(CAnimationCallbackHandler * pCallbackHandler)
{
	m_pAnimationCallbackHandler = pCallbackHandler;
}

void CAnimationTrack::HandleCallback()
{
	if (m_pAnimationCallbackHandler)
	{
		for (int i = 0; i < m_nCallbackKeys; i++)
		{
			if (::IsEqual(m_pCallbackKeys[i].m_fTime, m_fPosition, ANIMATION_CALLBACK_EPSILON))
			{
				if (m_pCallbackKeys[i].m_pCallbackData) m_pAnimationCallbackHandler->HandleCallback(m_pCallbackKeys[i].m_pCallbackData, m_fPosition);
				break;
			}
		}
	}
}

float CAnimationTrack::UpdatePosition(float fTrackPosition, float fElapsedTime, float fAnimationLength)
{
	float fTrackElapsedTime = fElapsedTime * m_fSpeed;
	switch (m_nType)
	{
	case ANIMATION_TYPE_LOOP:
	{
		if (m_fPosition < 0.0f) m_fPosition = 0.0f;
		else
		{
			m_fPosition = fTrackPosition + fTrackElapsedTime;
			if (m_fPosition > fAnimationLength)
			{
				m_fPosition = -ANIMATION_CALLBACK_EPSILON;
				m_iAnimLoopCount += 1;
				if (m_iAnimLoopEndCount < m_iAnimLoopCount)
				{
					m_iAnimLoopCount = 0;
					m_fAnimLoopTime = 0;
					m_bLoop = false;
				}
				return(fAnimationLength);
			}
		}
		//			m_fPosition = fmod(fTrackPosition, m_pfKeyFrameTimes[m_nKeyFrames-1]); // m_fPosition = fTrackPosition - int(fTrackPosition / m_pfKeyFrameTimes[m_nKeyFrames-1]) * m_pfKeyFrameTimes[m_nKeyFrames-1];
		//			m_fPosition = fmod(fTrackPosition, m_fLength); //if (m_fPosition < 0) m_fPosition += m_fLength;
		//			m_fPosition = fTrackPosition - int(fTrackPosition / m_fLength) * m_fLength;
		break;
	}
	case ANIMATION_TYPE_ONCE:
		m_fPosition = fTrackPosition + fTrackElapsedTime;
		if (m_fPosition > fAnimationLength) m_fPosition = fAnimationLength;
		break;
	case ANIMATION_TYPE_PINGPONG:
		break;
	}

	return(m_fPosition);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CAnimationController::CAnimationController(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nAnimationTracks, CLoadedModelInfo *pModel)
{
	m_nAnimationTracks = nAnimationTracks;
    m_pAnimationTracks = new CAnimationTrack[nAnimationTracks];

	m_pAnimationSets = pModel->m_pAnimationSets;
	m_pAnimationSets->AddRef();

	m_pModelRootObject = pModel->m_pModelRootObject;

	m_nSkinnedMeshes = pModel->m_nSkinnedMeshes;
	m_ppSkinnedMeshes = new CSkinnedMesh*[m_nSkinnedMeshes];
	for (int i = 0; i < m_nSkinnedMeshes; i++) m_ppSkinnedMeshes[i] = pModel->m_ppSkinnedMeshes[i];

	m_ppd3dcbSkinningBoneTransforms = new ID3D12Resource*[m_nSkinnedMeshes];
	m_ppcbxmf4x4MappedSkinningBoneTransforms = new XMFLOAT4X4*[m_nSkinnedMeshes];

	UINT ncbElementBytes = (((sizeof(XMFLOAT4X4) * SKINNED_ANIMATION_BONES) + 255) & ~255); //256의 배수
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
		m_ppd3dcbSkinningBoneTransforms[i]->Map(0, NULL, (void **)&m_ppcbxmf4x4MappedSkinningBoneTransforms[i]);
	}
}

CAnimationController::~CAnimationController()
{
	if (m_pAnimationTracks)
	{
		delete[] m_pAnimationTracks;
		m_pAnimationTracks = nullptr;
	}
		

	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppd3dcbSkinningBoneTransforms[i]->Unmap(0, NULL);
		m_ppd3dcbSkinningBoneTransforms[i]->Release();
	}
	if (m_ppd3dcbSkinningBoneTransforms) delete[] m_ppd3dcbSkinningBoneTransforms;
	if (m_ppcbxmf4x4MappedSkinningBoneTransforms) delete[] m_ppcbxmf4x4MappedSkinningBoneTransforms;

	if (m_pAnimationSets) m_pAnimationSets->Release();

	if (m_ppSkinnedMeshes) delete[] m_ppSkinnedMeshes;
}

void CAnimationController::SetCallbackKeys(int nAnimationTrack, int nCallbackKeys)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetCallbackKeys(nCallbackKeys);
}

void CAnimationController::SetCallbackKey(int nAnimationTrack, int nKeyIndex, float fKeyTime, void* pData)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetCallbackKey(nKeyIndex, fKeyTime, pData);
}

void CAnimationController::SetAnimationCallbackHandler(int nAnimationTrack, CAnimationCallbackHandler *pCallbackHandler)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetAnimationCallbackHandler(pCallbackHandler);
}

void CAnimationController::SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].m_nAnimationSet = nAnimationSet;
}

void CAnimationController::SetTrackEnable(int nAnimationTrack, bool bEnable)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetEnable(bEnable);
}

void CAnimationController::SetTrackPosition(int nAnimationTrack, float fPosition)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetPosition(fPosition);
}

void CAnimationController::SetTrackSpeed(int nAnimationTrack, float fSpeed)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetSpeed(fSpeed);
}

void CAnimationController::SetTrackWeight(int nAnimationTrack, float fWeight)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetWeight(fWeight);
}

void CAnimationController::SetTrackBlendingWeight(int nAnimationTrack, float fWeight)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetBlendingWeight(fWeight);
}

void CAnimationController::SetTrackContinuousAni(int nAnimationTrack, bool bContinuousAni)
{
	if (m_pAnimationTracks) m_pAnimationTracks[nAnimationTrack].SetContinuousAni(bContinuousAni);
}

void CAnimationController::SetDetailSkillAnim_Hero(int nAnimationTrack, int iSkillNum, int iAnimNum)
{
	float fSpeed = 1;
	int iType = ANIMATION_TYPE_ONCE;
	vector <int> vecSkill = SceneManager::GetInstance()->m_MatchingAniList[iSkillNum - 1];

	switch (iSkillNum)
	{
	case 1://Archer / BackStep
	case 2:// Archer / Dodge
	case 4:// Archer / Vault
		fSpeed = 2;
		break;
	case 6:// Archer / Sticky Arrow
		if(iAnimNum == vecSkill[0])
			fSpeed = 7;
		break;
	case 11:// Archer / Phoenix_Arrow
		if (iAnimNum == vecSkill[0])
			fSpeed = 3;
		break;
	case 12://Archer / Storm_Arrow
		iType = ANIMATION_TYPE_LOOP;
		m_pAnimationTracks[nAnimationTrack].m_fAnimLoopEndTime = 3.f;
		m_pAnimationTracks[nAnimationTrack].m_iAnimLoopEndCount = 5;
		break;
	case 13: //Fighter/순보
		fSpeed = 6;
		break;
	case 14://Fighter/회피기동
	case 15://Fighter/회전각
	case 21://Fighter/신풍각
		fSpeed = 2;
		break;
	case 17://Fighter/명왕권
		if (iAnimNum == vecSkill[0])
			fSpeed = 0.9f;
		break;
	case 18://Fighter/운기조식
		iType = ANIMATION_TYPE_LOOP;
		m_pAnimationTracks[nAnimationTrack].m_fAnimLoopEndTime = 3.f;
		m_pAnimationTracks[nAnimationTrack].m_iAnimLoopEndCount = 100;
		break;
	case 20://Fighter/금강불괴
		iType = ANIMATION_TYPE_LOOP;
		m_pAnimationTracks[nAnimationTrack].m_fAnimLoopEndTime = 100000000.f;
		m_pAnimationTracks[nAnimationTrack].m_iAnimLoopEndCount = 1000000;
		m_pAnimationTracks[nAnimationTrack].m_bOnOff = true;
		fSpeed = 0.5f;
		break;
	case 23://Fighter/장풍
		if (iAnimNum == vecSkill[0])
			fSpeed = 3.f;
		break;
	case 27://Swordman/Heavy_slash
		fSpeed = 1.2f;
		break;
	case 25://Swordman/dodging
	case 29://Swordman/war_cry
	case 31://Swordman/berserk
	case 32://Swordman/Aura_blade
		fSpeed = 2.f;
		break;
	case 30://Swordman/DefensiveStance
		m_pAnimationTracks[nAnimationTrack].m_bOnOff = true;
		break;
	case 33://Swordman/hell_blade
		iType = ANIMATION_TYPE_LOOP;
		m_pAnimationTracks[nAnimationTrack].m_fAnimLoopEndTime = 5.f;
		m_pAnimationTracks[nAnimationTrack].m_iAnimLoopEndCount = 5;
		break;
	case 35://Swordman/protected_area
		fSpeed = 1.3f;
		break;
	case 37://Wizard/Teleport
	case 39://Wizard/BodyStrength
		fSpeed = 2.f;
		break;
	case 40://Wizard/Enchant
		fSpeed = 1.5f;
		break;
	case 44://Wizard/MagicEye
		fSpeed = 1.5f;
		break;
	case 45://Wizard/DarknessRay
		fSpeed = 0.4f;
		break;
	case 47://Wizard/BigBang
		fSpeed = 1.5f;
		break;
	default:
		break;
	}


	if (m_pAnimationTracks)
	{
		m_pAnimationTracks[nAnimationTrack].SetSpeed(fSpeed);
		if (iType == ANIMATION_TYPE_LOOP)
			m_pAnimationTracks[nAnimationTrack].m_bLoop = true;
		m_pAnimationTracks[nAnimationTrack].m_nType = iType;
	}
}

void CAnimationController::SetDetailSkillAnim_Boss(int nAnimationTrack, int iSkillNum)
{
	float fSpeed = 1;
	int iType = ANIMATION_TYPE_ONCE;

	switch (iSkillNum)
	{
	case 2://Ogre/crunch
	case 4://Ogre/roar
	case 9://Ogre/Butting
		fSpeed = 2;
		break;
	case 3:
		iType = ANIMATION_TYPE_LOOP;
		m_pAnimationTracks[nAnimationTrack].m_fAnimLoopEndTime = 100.f;
		m_pAnimationTracks[nAnimationTrack].m_iAnimLoopEndCount = 100;
		break;
	case 18://Programmer/SwitchCaseLaser
	case 19://Programmer/WhileTrue
		m_pAnimationTracks[nAnimationTrack].m_bOnOff = true;
		break;
	case 20://Programmer/HelloWorld
		fSpeed = 2;
		break;
	default:
		break;
	}


	if (m_pAnimationTracks)
	{
		m_pAnimationTracks[nAnimationTrack].SetSpeed(fSpeed);
		if (iType == ANIMATION_TYPE_LOOP)
			m_pAnimationTracks[nAnimationTrack].m_bLoop = true;
		m_pAnimationTracks[nAnimationTrack].m_nType = iType;
	}
}


void CAnimationController::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	for (int i = 0; i < m_nSkinnedMeshes; i++)
	{
		m_ppSkinnedMeshes[i]->m_pd3dcbSkinningBoneTransforms = m_ppd3dcbSkinningBoneTransforms[i];
		m_ppSkinnedMeshes[i]->m_pcbxmf4x4MappedSkinningBoneTransforms = m_ppcbxmf4x4MappedSkinningBoneTransforms[i];
	}
}
/*
void CAnimationController::AdvanceTime(float fTimeElapsed, CGameObject *pRootGameObject) 
{
	m_fTime += fTimeElapsed; 
	if (m_pAnimationTracks)
	{
//		for (int k = 0; k < m_nAnimationTracks; k++) m_pAnimationTracks[k].m_fPosition += (fTimeElapsed * m_pAnimationTracks[k].m_fSpeed);
		for (int k = 0; k < m_nAnimationTracks; k++) m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet]->UpdatePosition(fTimeElapsed * m_pAnimationTracks[k].m_fSpeed);

		for (int j = 0; j < m_pAnimationSets->m_nAnimatedBoneFrames; j++)
		{
			XMFLOAT4X4 xmf4x4Transform = Matrix4x4::Zero();
			for (int k = 0; k < m_nAnimationTracks; k++)
			{
				if (m_pAnimationTracks[k].m_bEnable)
				{
					CAnimationSet *pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet];
					XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j);
					xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4TrackTransform, m_pAnimationTracks[k].m_fWeight));
				}
			}
			m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent = xmf4x4Transform;
		}

		pRootGameObject->UpdateTransform(NULL);

		for (int k = 0; k < m_nAnimationTracks; k++)
		{
			if (m_pAnimationTracks[k].m_bEnable) m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet]->HandleCallback();
		}
	}
} 
//*/
//*
void CAnimationController::AdvanceTime(float fTimeElapsed, CGameObject* pRootGameObject)
{
	m_fTime += fTimeElapsed;
	if (m_pAnimationTracks)
	{
		for (int j = 0; j < m_pAnimationSets->m_nAnimatedBoneFrames; j++) m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent = Matrix4x4::Zero();

		if (m_bBlend)
		{
			float fPlayTime = 0.1f;
			float w_increment = fTimeElapsed / fPlayTime;
			m_fProgressTime += fTimeElapsed;
			if(m_fInterpolateW < 1.f)
				m_fInterpolateW += w_increment;
			CAnimationSet* pPreAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[m_iPreTrackNum].m_nAnimationSet];
			CAnimationSet* pPostAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[m_iPostTrackNum].m_nAnimationSet];
			//float fPosition = m_pAnimationTracks[m_iPreTrackNum].UpdatePosition(m_pAnimationTracks[m_iPreTrackNum].m_fPosition, fTimeElapsed, pPreAnimationSet->m_fLength);
			//float fPosition = pPreAnimationSet->m_fLength;
			float fPosition = m_pAnimationTracks[m_iPreTrackNum].m_fPosition;
			for (int j = 0; j < m_pAnimationSets->m_nAnimatedBoneFrames; j++)
			{
				XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent;
				XMFLOAT4X4 xmf4x4PreTrackTransform = pPreAnimationSet->GetSRT(j, fPosition);
				XMFLOAT4X4 xmf4x4PostTrackTransform = pPostAnimationSet->GetSRT(j, 0);
				XMFLOAT4X4 xmf4x4BlendingTransform = Matrix4x4::Interpolate(xmf4x4PreTrackTransform, xmf4x4PostTrackTransform, m_fInterpolateW);
				xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4BlendingTransform, 1.0f));
				m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent = xmf4x4Transform;
			}
			//m_pAnimationTracks[k].HandleCallback();
			if (m_fProgressTime > fPlayTime)
			{
				m_fProgressTime = 0.f;
				m_fInterpolateW = 0.f;
				m_bBlend = false;
				m_pAnimationTracks[m_iPreTrackNum].m_fPosition = 0;
			}
		}
		else
		{
			for (int k = 0; k < m_nAnimationTracks; k++)
			{
				if (m_pAnimationTracks[k].m_bEnable)
				{
					if (m_pAnimationTracks[k].m_bLoop)
					{
						m_pAnimationTracks[k].m_fAnimLoopTime += fTimeElapsed;
						if (m_pAnimationTracks[k].m_fAnimLoopEndTime < m_pAnimationTracks[k].m_fAnimLoopTime)
						{
							m_pAnimationTracks[k].m_fAnimLoopTime = 0;
							m_pAnimationTracks[k].m_iAnimLoopCount = 0;
							m_pAnimationTracks[k].m_bLoop = false;
						}
					}
					CAnimationSet* pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[k].m_nAnimationSet];
					float fPosition = m_pAnimationTracks[k].UpdatePosition(m_pAnimationTracks[k].m_fPosition, fTimeElapsed, pAnimationSet->m_fLength);
					for (int j = 0; j < m_pAnimationSets->m_nAnimatedBoneFrames; j++)
					{
						XMFLOAT4X4 xmf4x4Transform = m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent;
						XMFLOAT4X4 xmf4x4TrackTransform = pAnimationSet->GetSRT(j, fPosition);
						xmf4x4Transform = Matrix4x4::Add(xmf4x4Transform, Matrix4x4::Scale(xmf4x4TrackTransform, m_pAnimationTracks[k].m_fWeight));
						m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j]->m_xmf4x4ToParent = xmf4x4Transform;
					}
					m_pAnimationTracks[k].HandleCallback();
				}
			}
		}
		pRootGameObject->UpdateTransform(NULL);

		OnRootMotion(pRootGameObject);
		OnAnimationIK(pRootGameObject);
	}
}
void CAnimationController::CheckingAniChage()
{
	for (int k = 0; k < m_nAnimationTracks; k++)
	{
		if (m_pAnimationTracks[k].m_bEnable)
		{
			if (m_iProgressAnimationTrack != k)
			{
				m_iProgressAnimationTrack = k;
				m_bAniChange = true;
			}
			else
				m_bAniChange = false;
		}
	}
}
bool CAnimationController::IsAnimationFinished(int nAnimationTrack) const
{
	CAnimationSet* pAnimationSet = m_pAnimationSets->m_pAnimationSets[m_pAnimationTracks[nAnimationTrack].m_nAnimationSet];
	float fTrackPosition = m_pAnimationTracks[nAnimationTrack].m_fPosition;
	float fAnimationLength = pAnimationSet->m_fLength;

	// Check if the current position is greater than or equal to the animation length
	if (m_pAnimationTracks[nAnimationTrack].m_bEnable && fTrackPosition >= fAnimationLength)
	{
		return true;
	}

	return false;
}
//*/
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CLoadedModelInfo::~CLoadedModelInfo()
{
	if (m_ppSkinnedMeshes) delete[] m_ppSkinnedMeshes;
}

void CLoadedModelInfo::PrepareSkinning()
{
	int nSkinnedMesh = 0;
	m_ppSkinnedMeshes = new CSkinnedMesh*[m_nSkinnedMeshes];
	m_pModelRootObject->FindAndSetSkinnedMesh(m_ppSkinnedMeshes, &nSkinnedMesh);

	for (int i = 0; i < m_nSkinnedMeshes; i++) m_ppSkinnedMeshes[i]->PrepareSkinning(m_pModelRootObject);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

const char* CGameObject::FrameNames[] = {
"", // dummy element at index 0
"Chr_HeadCoverings_Base_Hair",
"Chr_HeadCoverings_No_FacialHair",
"Chr_HeadCoverings_No_Hair",
"Chr_Hair",
"Chr_HelmetAttachment",
"Chr_BackAttachment",
"Chr_ShoulderAttachRight",
"Chr_ShoulderAttachLeft",
"Chr_ElbowAttachRight",
"Chr_ElbowAttachLeft",
"Chr_HipsAttachment",
"Chr_KneeAttachRight",
"Chr_KneeAttachLeft",
"Chr_Ear_Ear",

"Chr_Head",
"Chr_Head_No_Elements",
"Chr_Eyebrow",
"Chr_Torso",
"Chr_ArmUpperRight",
"Chr_ArmUpperLeft",
"Chr_ArmLowerRight",
"Chr_ArmLowerLeft",
"Chr_HandRight",
"Chr_HandLeft",
"Chr_Hips",
"Chr_LegRight",
"Chr_LegLeft"
};

CGameObject::CGameObject()
{
	m_xmf4x4ToParent = Matrix4x4::Identity();
	m_xmf4x4World = Matrix4x4::Identity();
}

CGameObject::CGameObject(int nMaterials) : CGameObject()
{
	m_nMaterials = nMaterials;
	if (m_nMaterials > 0)
	{
		m_ppMaterials = new CMaterial*[m_nMaterials];
		for(int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;
	}
}

CGameObject::~CGameObject()
{
	if (m_pMesh) m_pMesh->Release();

	if (m_nMaterials > 0)
	{
		for (int i = 0; i < m_nMaterials; i++)
		{
			if (m_ppMaterials[i])
			{
				m_ppMaterials[i]->Release();
				m_ppMaterials[i] = nullptr;
			}
		}
	}
	if (m_ppMaterials) delete[] m_ppMaterials;

	if (m_pSkinnedAnimationController) delete m_pSkinnedAnimationController;

	ReleaseShaderVariables();
}

void CGameObject::AddRef() 
{ 
	m_nReferences++; 

	if (m_pSibling) m_pSibling->AddRef();
	if (m_pChild) m_pChild->AddRef();
}

int CGameObject::Release() 
{ 
	int childRelease = 0;
	int siblingRelease = 0;

	if (m_pChild) {
		childRelease = m_pChild->Release();
	}
	if (m_pSibling) {
		siblingRelease = m_pSibling->Release();
	}

	if (--m_nReferences <= 0) {
		delete this;
		return 0;
	}
	else {
		return m_nReferences;
	}
}

void CGameObject::SetChild(CGameObject *pChild, bool bReferenceUpdate)
{
	if (pChild)
	{
		pChild->m_pParent = this;
		if (bReferenceUpdate) pChild->AddRef();
		iMyShareNum = pChild->iTotalShareNum;
		++(pChild->iTotalShareNum);
	}
	if (m_pChild)
	{
		if (pChild) pChild->m_pSibling = m_pChild->m_pSibling;
		m_pChild->m_pSibling = pChild;
	}
	else
	{
		m_pChild = pChild;
	}
}

void CGameObject::RemoveChild(CGameObject* pChild)
{
	// If the specified child is the first child, update the head pointer
	if (m_pChild == pChild) {
		m_pChild = pChild->m_pSibling;
		pChild->m_pSibling = NULL;
	}
	// Otherwise, traverse the sibling list to find the specified child
	else {
		CGameObject* pSibling = m_pChild;
		while (pSibling) {
			if (pSibling->m_pSibling == pChild) {
				pSibling->m_pSibling = pChild->m_pSibling;
				pChild->m_pSibling = NULL;
				break;
			}
			pSibling = pSibling->m_pSibling;
		}
	}
}

void CGameObject::SetMesh(CMesh *pMesh)
{
	if (pMesh) pMesh->AddRef();
	if (m_pMesh) m_pMesh->Release();
	m_pMesh = pMesh;
	m_sharedMesh.reset();
}

void CGameObject::SetSharedMesh(const std::shared_ptr<CMesh>& mesh)
{
	// 같은 핸들을 다시 지정해도 SetMesh의 reset으로 소유자가 사라지지 않게 한다.
	const auto owner = mesh;
	SetMesh(owner.get());
	m_sharedMesh = owner;
}

void CGameObject::SetShader(CShader *pShader)
{
	m_nMaterials = 1;
	m_ppMaterials = new CMaterial*[m_nMaterials];
	m_ppMaterials[0] = new CMaterial(0);
	m_ppMaterials[0]->SetShader(pShader);
}

void CGameObject::SetShader(int nMaterial, CShader *pShader)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->SetShader(pShader);
}

void CGameObject::SetRootShader(CShader* pShader)
{
	for (int i = 0; i < m_nMaterials; ++i)
	{
		m_ppMaterials[i]->SetShader(pShader);
	}
	
	if (m_pSibling) m_pSibling->SetRootShader(pShader);
	if (m_pChild) m_pChild->SetRootShader(pShader);
}

void CGameObject::SetMaterial(int nMaterial, CMaterial *pMaterial)
{
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->Release();
	m_ppMaterials[nMaterial] = pMaterial;
	if (m_ppMaterials[nMaterial]) m_ppMaterials[nMaterial]->AddRef();
}

CSkinnedMesh *CGameObject::FindSkinnedMesh(char *pstrSkinnedMeshName)
{
	CSkinnedMesh *pSkinnedMesh = NULL;
	if (m_pMesh && (m_pMesh->GetType() & VERTEXT_BONE_INDEX_WEIGHT)) 
	{
		pSkinnedMesh = (CSkinnedMesh *)m_pMesh;
		if(!strncmp(pSkinnedMesh->m_pstrMeshName, pstrSkinnedMeshName, strlen(pstrSkinnedMeshName))) return(pSkinnedMesh);
	}

	if (m_pSibling) if (pSkinnedMesh = m_pSibling->FindSkinnedMesh(pstrSkinnedMeshName)) return(pSkinnedMesh);
	if (m_pChild) if (pSkinnedMesh = m_pChild->FindSkinnedMesh(pstrSkinnedMeshName)) return(pSkinnedMesh);

	return(NULL);
}

void CGameObject::FindAndSetSkinnedMesh(CSkinnedMesh **ppSkinnedMeshes, int *pnSkinnedMesh)
{
	if (m_pMesh && (m_pMesh->GetType() & VERTEXT_BONE_INDEX_WEIGHT)) ppSkinnedMeshes[(*pnSkinnedMesh)++] = (CSkinnedMesh *)m_pMesh;

	if (m_pSibling) m_pSibling->FindAndSetSkinnedMesh(ppSkinnedMeshes, pnSkinnedMesh);
	if (m_pChild) m_pChild->FindAndSetSkinnedMesh(ppSkinnedMeshes, pnSkinnedMesh);
}

CGameObject *CGameObject::FindFrame(const char *pstrFrameName)
{
	CGameObject *pFrameObject = NULL;
	if (!strncmp(m_pstrFrameName, pstrFrameName, strlen(pstrFrameName))) return(this);

	if (m_pSibling) if (pFrameObject = m_pSibling->FindFrame(pstrFrameName)) return(pFrameObject);
	if (m_pChild) if (pFrameObject = m_pChild->FindFrame(pstrFrameName)) return(pFrameObject);

	return(NULL);
}

void CGameObject::UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent)
{
	m_xmf4x4World = (pxmf4x4Parent) ? Matrix4x4::Multiply(m_xmf4x4ToParent, *pxmf4x4Parent) : m_xmf4x4ToParent;

	if (m_pSibling) m_pSibling->UpdateTransform(pxmf4x4Parent);
	if (m_pChild) m_pChild->UpdateTransform(&m_xmf4x4World);
}

void CGameObject::SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet)
{
	if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->SetTrackAnimationSet(nAnimationTrack, nAnimationSet);
}

void CGameObject::SetTrackAnimationPosition(int nAnimationTrack, float fPosition)
{
	if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->SetTrackPosition(nAnimationTrack, fPosition);
}

void CGameObject::Animate(float fTimeElapsed)
{
	OnPrepareRender();

	if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->AdvanceTime(fTimeElapsed, this);

	if (m_pSibling) m_pSibling->Animate(fTimeElapsed);
	if (m_pChild) m_pChild->Animate(fTimeElapsed);
}

void CGameObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int SharedNum, int nPipelineState)
{
	UpdateShaderVariables(pd3dCommandList);

	bool bCurling = true;
	switch (SceneManager::GetInstance()->m_nCurScene)
	{
	case SCENEKIND::TITLE:
		break;
	case SCENEKIND::LOBBY:
		//bCurling = Frustum::GetInstance()->CheckPoint(GetPosition()) || m_eObjType != OBJ_TYPE::OBJECT;
		bCurling = true;
		break;
	case SCENEKIND::READY:
		break;
	case SCENEKIND::INGAME:
		bCurling = Frustum::GetInstance()->CheckSphere(GetPosition(), m_fMaxRadius * 1.1f) || m_eObjType != OBJ_TYPE::OBJECT;
		//bCurling = Frustum::GetInstance()->CheckPoint(GetPosition()) || m_eObjType != OBJ_TYPE::OBJECT;
		//bCurling = true;
		break;
	default:
		break;
	}

	if(bCurling)
	{
		if (m_bIsRender)
		{
			if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->UpdateShaderVariables(pd3dCommandList);

			if (m_pMesh)
			{
				UpdateShaderVariable(pd3dCommandList, &m_xmf4x4World, SharedNum);

				if (m_nMaterials > 0)
				{
					for (int i = 0; i < m_nMaterials; i++)
					{
						if (m_ppMaterials[i])
						{
							if (m_ppMaterials[i]->m_pShader) m_ppMaterials[i]->m_pShader->Render(pd3dCommandList, pCamera, nPipelineState);
							m_ppMaterials[i]->UpdateShaderVariable(pd3dCommandList);
						}

						if (m_pMesh)m_pMesh->Render(pd3dCommandList, i);
					}
				}
			}
		}
	}



	if (m_pSibling) m_pSibling->Render(pd3dCommandList, pCamera, SharedNum, nPipelineState);
	if (m_pChild) m_pChild->Render(pd3dCommandList, pCamera, SharedNum, nPipelineState);
}

void CGameObject::PureRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	bool bCurling = true;
	switch (SceneManager::GetInstance()->m_nCurScene)
	{
	case SCENEKIND::TITLE:
		break;
	case SCENEKIND::LOBBY:
		bCurling = Frustum::GetInstance()->CheckPoint(GetPosition()) || m_eObjType != OBJ_TYPE::OBJECT;
		//bCurling = true;
		break;
	case SCENEKIND::READY:
		break;
	case SCENEKIND::INGAME:
		bCurling = Frustum::GetInstance()->CheckSphere(GetPosition(), m_fMaxRadius) || m_eObjType != OBJ_TYPE::OBJECT;
		//bCurling = Frustum::GetInstance()->CheckPoint(GetPosition()) || m_eObjType != OBJ_TYPE::OBJECT;
		//bCurling = true;
		break;
	default:
		break;
	}

	if (bCurling)
	{
		if (m_bIsRender)
		{
			if (m_pSkinnedAnimationController) m_pSkinnedAnimationController->UpdateShaderVariables(pd3dCommandList);

			if (m_pMesh)
			{
				UpdateShaderVariable(pd3dCommandList, &m_xmf4x4World);

				if (m_nMaterials > 0)
				{
					for (int i = 0; i < m_nMaterials; i++)
					{
						if (m_ppMaterials[i])
						{
							if (m_ppMaterials[i]->m_pShader) m_ppMaterials[i]->m_pShader->Render(pd3dCommandList, pCamera);
							m_ppMaterials[i]->UpdateShaderVariable(pd3dCommandList);
						}

						if (m_pMesh)m_pMesh->Render(pd3dCommandList, i);
					}
				}
			}
		}
	}
}

void CGameObject::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	//UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256의 배수
	//m_pd3dcbDissolve = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	//m_pd3dcbDissolve->Map(0, NULL, (void**)&m_pObjectState);

	UINT ncbElementBytes = ((sizeof(OBJECT_INFO) + 255) & ~255); //256의 배수
	//m_pd3dcbObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	//m_pd3dcbObject->Map(0, NULL, (void**)&m_pcbMappedObjects);

	ID3D12Resource* m_pd3dcbObject = NULL;
	OBJECT_INFO* m_pcbMappedObjects = NULL;

	m_pd3dcbObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbObject->Map(0, NULL, (void**)&m_pcbMappedObjects);

	m_pd3dcbVecObject.push_back(m_pd3dcbObject);
	m_pcbMappedVecObjects.push_back(m_pcbMappedObjects);


	if (m_ppMaterials)
	{
		for (int i = 0; i < m_nMaterials; ++i)
		{
			m_ppMaterials[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
		}
	}

}

void CGameObject::AllCreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	if (m_pSibling) m_pSibling->AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
	if (m_pChild) m_pChild->AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World, int SharedNum)
{
	XMFLOAT4X4 xmf4x4World;
	XMStoreFloat4x4(&xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(pxmf4x4World)));
	//pd3dCommandList->SetGraphicsRoot32BitConstants(1, 16, &xmf4x4World, 0);
	//pd3dCommandList->SetGraphicsRoot32BitConstants(1, 1, &m_nObjectID, 33);
	//pd3dCommandList->SetGraphicsRoot32BitConstants(1, 1, &m_nObjectDissolveState, 34);

	
	/*D3D12_GPU_VIRTUAL_ADDRESS d3dcbObjectsGpuVirtualAddress = m_pd3dcbObject->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(1, d3dcbObjectsGpuVirtualAddress);*/


	D3D12_GPU_VIRTUAL_ADDRESS d3dcbObjectsGpuVirtualAddress = m_pd3dcbVecObject[SharedNum]->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(1, d3dcbObjectsGpuVirtualAddress);

	::memcpy(&m_pcbMappedVecObjects[SharedNum]->m_xmf4x4World, &xmf4x4World, sizeof(XMFLOAT4X4));
	::memcpy(&m_pcbMappedVecObjects[SharedNum]->m_nObjectID, &m_nObjectID, sizeof(UINT));
	::memcpy(&m_pcbMappedVecObjects[SharedNum]->m_nObjectDissolveState, &m_nObjectDissolveState, sizeof(float));

}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial)
{
}

void CGameObject::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, bool dissovle)
{
	//::memcpy(m_pObjectState, &m_nObjectState, sizeof(UINT));

	//D3D12_GPU_VIRTUAL_ADDRESS d3dcbDissolveGpuVirtualAddress = m_pd3dcbDissolve->GetGPUVirtualAddress();
	//pd3dCommandList->SetGraphicsRootConstantBufferView(13, d3dcbDissolveGpuVirtualAddress);
}

void CGameObject::ReleaseShaderVariables()
{
	if (m_pd3dcbVecObject.size())
	{
		for (auto p : m_pd3dcbVecObject)
		{
			p->Unmap(0, NULL);
			p->Release();
			p = NULL;
		}
		m_pd3dcbVecObject.clear();
	}

}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_pMesh) m_pMesh->ReleaseUploadBuffers();

	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i]) {
			m_ppMaterials[i]->ReleaseUploadBuffers();
		}
	}

	if (m_pSibling) m_pSibling->ReleaseUploadBuffers();
	if (m_pChild) m_pChild->ReleaseUploadBuffers();
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4ToParent._41 = x;
	m_xmf4x4ToParent._42 = y;
	m_xmf4x4ToParent._43 = z;

	UpdateTransform(NULL);
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

void CGameObject::SetScreenPosition(XMFLOAT2 xmf2Position)
{
	float xOffset = (float)(FRAME_BUFFER_WIDTH / 2);
	float yOffset = (float)(FRAME_BUFFER_HEIGHT / 2);

	float xPos = xmf2Position.x - xOffset;
	float yPos = yOffset - xmf2Position.y;

	float textureWidth = m_pMesh->m_fWidth;
	float textureHeight = m_pMesh->m_fHeight;
	xPos += textureWidth / 2.0f;
	yPos -= textureHeight / 2.0f;

	m_xmf4x4ToParent._41 = xPos;
	m_xmf4x4ToParent._42 = yPos;
	m_xmf4x4ToParent._43 = 0.0f;

	UpdateTransform(NULL);
}

void CGameObject::Move(XMFLOAT3 xmf3Offset)
{
	m_xmf4x4ToParent._41 += xmf3Offset.x;
	m_xmf4x4ToParent._42 += xmf3Offset.y;
	m_xmf4x4ToParent._43 += xmf3Offset.z;

	UpdateTransform(NULL);
}

void CGameObject::SetScale(float x, float y, float z)
{
	XMMATRIX mtxScale = XMMatrixScaling(x, y, z);
	m_xmf4x4ToParent = Matrix4x4::Multiply(mtxScale, m_xmf4x4ToParent);

	UpdateTransform(NULL);
}

void CGameObject::SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3&& xmf3Up)
{
	XMFLOAT3 xmf3Position(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43);
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(xmf3Position, xmf3Target, xmf3Up);
	m_xmf4x4World._11 = mtxLookAt._11; m_xmf4x4World._12 = mtxLookAt._21; m_xmf4x4World._13 = mtxLookAt._31;
	m_xmf4x4World._21 = mtxLookAt._12; m_xmf4x4World._22 = mtxLookAt._22; m_xmf4x4World._23 = mtxLookAt._32;
	m_xmf4x4World._31 = mtxLookAt._13; m_xmf4x4World._32 = mtxLookAt._23; m_xmf4x4World._33 = mtxLookAt._33;
}


XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetToParentPosition()
{
	return(XMFLOAT3(m_xmf4x4ToParent._41, m_xmf4x4ToParent._42, m_xmf4x4ToParent._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

XMFLOAT3 CGameObject::GetScale()
{
	XMFLOAT3 scale;
	XMVECTOR scaleX, scaleY, scaleZ;
	XMMatrixDecompose(&scaleX, &scaleY, &scaleZ, XMLoadFloat4x4(&m_xmf4x4ToParent));

	scale.x = XMVectorGetX(scaleX);
	scale.y = XMVectorGetY(scaleY);
	scale.z = XMVectorGetZ(scaleZ);

	return scale;
}

XMFLOAT2 CGameObject::GetScreenPosition()
{
	float xOffset = static_cast<float>(FRAME_BUFFER_WIDTH / 2);
	float yOffset = static_cast<float>(FRAME_BUFFER_HEIGHT / 2);

	float xPos = m_xmf4x4ToParent._41 - (m_pMesh->m_fWidth / 2.0f);
	float yPos = m_xmf4x4ToParent._42 + (m_pMesh->m_fHeight / 2.0f);

	float screenX = xPos + xOffset;
	float screenY = yOffset - yPos;

	return XMFLOAT2(screenX, screenY);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4ToParent = Matrix4x4::Multiply(mtxRotate, m_xmf4x4ToParent);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(XMFLOAT3 *pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4ToParent = Matrix4x4::Multiply(mtxRotate, m_xmf4x4ToParent);

	UpdateTransform(NULL);
}

void CGameObject::Rotate(XMFLOAT4 *pxmf4Quaternion)
{
	XMMATRIX mtxRotate = XMMatrixRotationQuaternion(XMLoadFloat4(pxmf4Quaternion));
	m_xmf4x4ToParent = Matrix4x4::Multiply(mtxRotate, m_xmf4x4ToParent);

	UpdateTransform(NULL);
}

//#define _WITH_DEBUG_FRAME_HIERARCHY

CTexture *CGameObject::FindReplicatedTexture(_TCHAR *pstrTextureName)
{
	for (int i = 0; i < m_nMaterials; i++)
	{
		if (m_ppMaterials[i])
		{
			for (int j = 0; j < m_ppMaterials[i]->m_nTextures; j++)
			{
				if (m_ppMaterials[i]->m_ppTextures[j])
				{
					if (!_tcsncmp(m_ppMaterials[i]->m_ppstrTextureNames[j], pstrTextureName, _tcslen(pstrTextureName))) {
						return(m_ppMaterials[i]->m_ppTextures[j]);
					}

				}
			}
		}
	}
	CTexture *pTexture = NULL;
	if (m_pSibling) if (pTexture = m_pSibling->FindReplicatedTexture(pstrTextureName)) return(pTexture);
	if (m_pChild) if (pTexture = m_pChild->FindReplicatedTexture(pstrTextureName)) return(pTexture);

	return(NULL);
}

int ReadIntegerFromFile(FILE *pInFile)
{
	int nValue = 0;
	UINT nReads = (UINT)::fread(&nValue, sizeof(int), 1, pInFile); 
	return(nValue);
}

float ReadFloatFromFile(FILE *pInFile)
{
	float fValue = 0;
	UINT nReads = (UINT)::fread(&fValue, sizeof(float), 1, pInFile); 
	return(fValue);
}

BYTE ReadStringFromFile(FILE *pInFile, char *pstrToken)
{
	BYTE nStrLength = 0;
	UINT nReads = 0;
	nReads = (UINT)::fread(&nStrLength, sizeof(BYTE), 1, pInFile);
	nReads = (UINT)::fread(pstrToken, sizeof(char), nStrLength, pInFile); 
	pstrToken[nStrLength] = '\0';

	return(nStrLength);
}

void CGameObject::LoadMaterialsFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CGameObject *pParent, FILE *pInFile, CShader *pShader)
{
	char pstrToken[64] = { '\0' };
	int nMaterial = 0;
	UINT nReads = 0;

	m_nMaterials = ReadIntegerFromFile(pInFile);

	m_ppMaterials = new CMaterial*[m_nMaterials];
	for (int i = 0; i < m_nMaterials; i++) m_ppMaterials[i] = NULL;

	CMaterial *pMaterial = NULL;

	for ( ; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);

		if (!strcmp(pstrToken, "<Material>:"))
		{
			nMaterial = ReadIntegerFromFile(pInFile);

			pMaterial = new CMaterial(7); //0:Albedo, 1:Specular, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal

			if (!pShader)
			{
				UINT nMeshType = GetMeshType();
				if (nMeshType & VERTEXT_NORMAL_TANGENT_TEXTURE)
				{
					if (nMeshType & VERTEXT_BONE_INDEX_WEIGHT)
					{
						pMaterial->SetSkinnedAnimationShader();
					}
					else
					{
						pMaterial->SetStandardShader();
					}
				}
			}
			SetMaterial(nMaterial, pMaterial);
		}
		else if (!strcmp(pstrToken, "<AlbedoColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4AlbedoColor), sizeof(float), 4, pInFile);
		}
		if (!strcmp(pstrToken, "<EmissiveColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4EmissiveColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularColor>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_xmf4SpecularColor), sizeof(float), 4, pInFile);
		}
		else if (!strcmp(pstrToken, "<Glossiness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossiness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Smoothness>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSmoothness), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<Metallic>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fSpecularHighlight), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<SpecularHighlight>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fMetallic), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<GlossyReflection>:"))
		{
			nReads = (UINT)::fread(&(pMaterial->m_fGlossyReflection), sizeof(float), 1, pInFile);
		}
		else if (!strcmp(pstrToken, "<AlbedoMap>:"))
		{
			pMaterial->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_ALBEDO_MAP, 3, pMaterial->m_ppstrTextureNames[0], &(pMaterial->m_ppTextures[0]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<SpecularMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_SPECULAR_MAP, 4, pMaterial->m_ppstrTextureNames[1], &(pMaterial->m_ppTextures[1]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<NormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_NORMAL_MAP, 5, pMaterial->m_ppstrTextureNames[2], &(pMaterial->m_ppTextures[2]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<MetallicMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_METALLIC_MAP, 6, pMaterial->m_ppstrTextureNames[3], &(pMaterial->m_ppTextures[3]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<EmissionMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_EMISSION_MAP, 7, pMaterial->m_ppstrTextureNames[4], &(pMaterial->m_ppTextures[4]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailAlbedoMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_ALBEDO_MAP, 8, pMaterial->m_ppstrTextureNames[5], &(pMaterial->m_ppTextures[5]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<DetailNormalMap>:"))
		{
			m_ppMaterials[nMaterial]->LoadTextureFromFile(pd3dDevice, pd3dCommandList, MATERIAL_DETAIL_NORMAL_MAP, 9, pMaterial->m_ppstrTextureNames[6], &(pMaterial->m_ppTextures[6]), pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "</Materials>"))
		{
			break;
		}
	}
}

CGameObject *CGameObject::LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader *pShader, int *pnSkinnedMeshes)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nFrame = 0, nTextures = 0;

	CGameObject *pGameObject = new CGameObject();

	for ( ; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<Frame>:"))
		{
			nFrame = ::ReadIntegerFromFile(pInFile);
			nTextures = ::ReadIntegerFromFile(pInFile);

			::ReadStringFromFile(pInFile, pGameObject->m_pstrFrameName);
		}
		else if (!strcmp(pstrToken, "<Transform>:"))
		{
			XMFLOAT3 xmf3Position, xmf3Rotation, xmf3Scale;
			XMFLOAT4 xmf4Rotation;
			nReads = (UINT)::fread(&xmf3Position, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf3Rotation, sizeof(float), 3, pInFile); //Euler Angle
			nReads = (UINT)::fread(&xmf3Scale, sizeof(float), 3, pInFile);
			nReads = (UINT)::fread(&xmf4Rotation, sizeof(float), 4, pInFile); //Quaternion
		}
		else if (!strcmp(pstrToken, "<TransformMatrix>:"))
		{
			nReads = (UINT)::fread(&pGameObject->m_xmf4x4ToParent, sizeof(float), 16, pInFile);
		}
		else if (!strcmp(pstrToken, "<Mesh>:"))
		{
			pGameObject->SetSharedMesh(CStandardMesh::LoadSharedGeometryFromFile(pd3dDevice, pd3dCommandList, pInFile));
		}
		else if (!strcmp(pstrToken, "<SkinningInfo>:"))
		{
			if (pnSkinnedMeshes) (*pnSkinnedMeshes)++;

			CSkinnedMesh *pSkinnedMesh = new CSkinnedMesh(pd3dDevice, pd3dCommandList);
			pSkinnedMesh->LoadSkinInfoFromFile(pd3dDevice, pd3dCommandList, pInFile);
			pSkinnedMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);

			::ReadStringFromFile(pInFile, pstrToken); //<Mesh>:
			if (!strcmp(pstrToken, "<Mesh>:")) pSkinnedMesh->LoadSharedGeometryDataFromFile(pd3dDevice, pd3dCommandList, pInFile);

			pGameObject->SetMesh(pSkinnedMesh);
		}
		else if (!strcmp(pstrToken, "<Materials>:"))
		{
			pGameObject->LoadMaterialsFromFile(pd3dDevice, pd3dCommandList, pParent, pInFile, pShader);
		}
		else if (!strcmp(pstrToken, "<Children>:"))
		{
			int nChilds = ::ReadIntegerFromFile(pInFile);
			if (nChilds > 0)
			{
				for (int i = 0; i < nChilds; i++)
				{
					CGameObject *pChild = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pGameObject, pInFile, pShader, pnSkinnedMeshes);
					if (pChild) pGameObject->SetChild(pChild);
#ifdef _WITH_DEBUG_FRAME_HIERARCHY
					TCHAR pstrDebug[256] = { 0 };
					_stprintf_s(pstrDebug, 256, "(Frame: %p) (Parent: %p)\n"), pChild, pGameObject);
					OutputDebugString(pstrDebug);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</Frame>"))
		{
			break;
		}
	}
	//Find required radius value for curling
	XMFLOAT3 extent = XMFLOAT3(0,0,0);
	if(pGameObject->m_pMesh)
		extent = pGameObject->m_pMesh->GetAABBExtents();
	XMFLOAT3 objectScale = pGameObject->GetScale();
	float MultiplyX = extent.x * objectScale.x;
	float MultiplyY = extent.y * objectScale.y;
	float MultiplyZ = extent.z * objectScale.z;

	pGameObject->m_fMaxRadius = MultiplyX;
	if (MultiplyY > pGameObject->m_fMaxRadius)
		pGameObject->m_fMaxRadius = MultiplyY;
	if (MultiplyZ > pGameObject->m_fMaxRadius)
		pGameObject->m_fMaxRadius = MultiplyZ;

	return(pGameObject);
}

void CGameObject::PrintFrameInfo(CGameObject *pGameObject, CGameObject *pParent)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("(Frame: %p) (Parent: %p)\n"), pGameObject, pParent);
	OutputDebugString(pstrDebug);

	if (pGameObject->m_pSibling) CGameObject::PrintFrameInfo(pGameObject->m_pSibling, pParent);
	if (pGameObject->m_pChild) CGameObject::PrintFrameInfo(pGameObject->m_pChild, pGameObject);
}

void CGameObject::SetObjectType(OBJ_TYPE eType)
{
	m_eObjType = eType;
	if (m_pChild)m_pChild->SetObjectType(eType);
	if (m_pSibling)m_pSibling->SetObjectType(eType);
}

void CGameObject::SetDissolveState(float fAmount)
{
	m_nObjectDissolveState = fAmount; 
	if (m_pChild)m_pChild->SetDissolveState(fAmount);
	if (m_pSibling)m_pSibling->SetDissolveState(fAmount);

}

void CGameObject::SetAddDissolveState(float fAmount)
{
	m_nObjectDissolveState += fAmount;
	if (m_pChild)m_pChild->SetAddDissolveState(fAmount);
	if (m_pSibling)m_pSibling->SetAddDissolveState(fAmount);
}

void CGameObject::DrawOff()
{
	m_bIsRender = false; // 현재 객체의 m_bIsRender를 False로 설정

   // 자식 객체의 m_bIsRender를 False로 설정
	if (m_pChild) m_pChild->DrawOff();

	// 형제 객체의 m_bIsRender를 False로 설정
	if (m_pSibling) m_pSibling->DrawOff();
}

void CGameObject::DrawOn()
{
	m_bIsRender = true; // 현재 객체의 m_bIsRender를 True로 설정

	// 자식 객체의 m_bIsRender를 True로 설정
	if (m_pChild) m_pChild->DrawOn();

	// 형제 객체의 m_bIsRender를 True로 설정
	if (m_pSibling) m_pSibling->DrawOn();
}

void CGameObject::SetObjectID(UINT id)
{
	m_nObjectID = id;
	if (m_pChild)m_pChild->SetObjectID(id);
	if (m_pSibling)m_pSibling->SetObjectID(id);
}

void CGameObject::LoadAnimationFromFile(FILE *pInFile, CLoadedModelInfo *pLoadedModel)
{
	char pstrToken[64] = { '\0' };
	UINT nReads = 0;

	int nAnimationSets = 0;

	for ( ; ; )
	{
		::ReadStringFromFile(pInFile, pstrToken);
		if (!strcmp(pstrToken, "<AnimationSets>:"))
		{
			nAnimationSets = ::ReadIntegerFromFile(pInFile);
			pLoadedModel->m_pAnimationSets = new CAnimationSets(nAnimationSets);
		}
		else if (!strcmp(pstrToken, "<FrameNames>:"))
		{
			pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames = ::ReadIntegerFromFile(pInFile); 
			pLoadedModel->m_pAnimationSets->m_ppAnimatedBoneFrameCaches = new CGameObject*[pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames];

			for (int j = 0; j < pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames; j++)
			{
				::ReadStringFromFile(pInFile, pstrToken);
				pLoadedModel->m_pAnimationSets->m_ppAnimatedBoneFrameCaches[j] = pLoadedModel->m_pModelRootObject->FindFrame(pstrToken);

#ifdef _WITH_DEBUG_SKINNING_BONE
				TCHAR pstrDebug[256] = { 0 };
				TCHAR pwstrAnimationBoneName[64] = { 0 };
				TCHAR pwstrBoneCacheName[64] = { 0 };
				size_t nConverted = 0;
				mbstowcs_s(&nConverted, pwstrAnimationBoneName, 64, pstrToken, _TRUNCATE);
				mbstowcs_s(&nConverted, pwstrBoneCacheName, 64, pLoadedModel->m_ppAnimatedBoneFrameCaches[j]->m_pstrFrameName, _TRUNCATE);
				_stprintf_s(pstrDebug, 256, _T("AnimationBoneFrame:: Cache(%s) AnimationBone(%s)\n"), pwstrBoneCacheName, pwstrAnimationBoneName);
				OutputDebugString(pstrDebug);
#endif
			}
		}
		else if (!strcmp(pstrToken, "<AnimationSet>:"))
		{
			int nAnimationSet = ::ReadIntegerFromFile(pInFile);

			::ReadStringFromFile(pInFile, pstrToken); //Animation Set Name

			float fLength = ::ReadFloatFromFile(pInFile);
			int nFramesPerSecond = ::ReadIntegerFromFile(pInFile);
			int nKeyFrames = ::ReadIntegerFromFile(pInFile);

			pLoadedModel->m_pAnimationSets->m_pAnimationSets[nAnimationSet] = new CAnimationSet(fLength, nFramesPerSecond, nKeyFrames, pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames, pstrToken);

			for (int i = 0; i < nKeyFrames; i++)
			{
				::ReadStringFromFile(pInFile, pstrToken);
				if (!strcmp(pstrToken, "<Transforms>:"))
				{
					CAnimationSet *pAnimationSet = pLoadedModel->m_pAnimationSets->m_pAnimationSets[nAnimationSet];

					int nKey = ::ReadIntegerFromFile(pInFile); //i
					float fKeyTime = ::ReadFloatFromFile(pInFile);

#ifdef _WITH_ANIMATION_SRT
					m_pfKeyFrameScaleTimes[i] = fKeyTime;
					m_pfKeyFrameRotationTimes[i] = fKeyTime;
					m_pfKeyFrameTranslationTimes[i] = fKeyTime;
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf3KeyFrameScales[i], sizeof(XMFLOAT3), pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames, pInFile);
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf4KeyFrameRotations[i], sizeof(XMFLOAT4), pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames, pInFile);
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf3KeyFrameTranslations[i], sizeof(XMFLOAT3), pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames, pInFile);
#else
					pAnimationSet->m_pfKeyFrameTimes[i] = fKeyTime;
					nReads = (UINT)::fread(pAnimationSet->m_ppxmf4x4KeyFrameTransforms[i], sizeof(XMFLOAT4X4), pLoadedModel->m_pAnimationSets->m_nAnimatedBoneFrames, pInFile);
#endif
				}
			}
		}
		else if (!strcmp(pstrToken, "</AnimationSets>"))
		{
			break;
		}
	}
}

CLoadedModelInfo *CGameObject::LoadGeometryAndAnimationFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, const char *pstrFileName, CShader *pShader)
{
	FILE *pInFile = NULL;
	::fopen_s(&pInFile, pstrFileName, "rb");
	::rewind(pInFile);

	CLoadedModelInfo *pLoadedModel = new CLoadedModelInfo();

	char pstrToken[64] = { '\0' };

	for ( ; ; )
	{
		if (::ReadStringFromFile(pInFile, pstrToken))
		{
			if (!strcmp(pstrToken, "<Hierarchy>:"))
			{
				pLoadedModel->m_pModelRootObject = CGameObject::LoadFrameHierarchyFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, NULL, pInFile, pShader, &pLoadedModel->m_nSkinnedMeshes);
				::ReadStringFromFile(pInFile, pstrToken); //"</Hierarchy>"
			}
			else if (!strcmp(pstrToken, "<Animation>:"))
			{
				CGameObject::LoadAnimationFromFile(pInFile, pLoadedModel);
				pLoadedModel->PrepareSkinning();
			}
			else if (!strcmp(pstrToken, "</Animation>:"))
			{
				break;
			}
		}
		else
		{
			break;
		}
	}

#ifdef _WITH_DEBUG_FRAME_HIERARCHY
	TCHAR pstrDebug[256] = { 0 };
	_stprintf_s(pstrDebug, 256, "Frame Hierarchy\n"));
	OutputDebugString(pstrDebug);

	CGameObject::PrintFrameInfo(pGameObject, NULL);
#endif

	return(pLoadedModel);
}



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CSkyBox::CSkyBox(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature) : CGameObject(1)
{
	CSkyBoxMesh *pSkyBoxMesh = new CSkyBoxMesh(pd3dDevice, pd3dCommandList, 20.0f, 20.0f, 2.0f);
	SetMesh(pSkyBoxMesh);	

	CTexture* pSkyBoxTexture = new CTexture(1, RESOURCE_TEXTURE_CUBE, 0, 1);
	pSkyBoxTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"SkyBox/Space.dds", RESOURCE_TEXTURE_CUBE, 0);

	CSkyBoxShader *pSkyBoxShader = new CSkyBoxShader();
	pSkyBoxShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	//pSkyBoxShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CScene::CreateShaderResourceViews(pd3dDevice, pSkyBoxTexture, 0, 10);

	CMaterial *pSkyBoxMaterial = new CMaterial(1);
	pSkyBoxMaterial->SetTexture(pSkyBoxTexture);
	pSkyBoxMaterial->SetShader(pSkyBoxShader);

	SetMaterial(0, pSkyBoxMaterial);
	SetObjectType(OBJ_TYPE::TEXTURE);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CSkyBox::~CSkyBox()
{
}

void CSkyBox::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int SharedNum, int nPipelineState)
{
	XMFLOAT3 xmf3CameraPos = pCamera->GetPosition();
	SetPosition(xmf3CameraPos.x, xmf3CameraPos.y, xmf3CameraPos.z);

	CGameObject::Render(pd3dCommandList, pCamera, SharedNum, nPipelineState);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
//

CMap::CMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks)
{
	CLoadedModelInfo* pEagleModel = pModel;
	if (!pEagleModel) pEagleModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Plane1.bin", NULL);

	SetChild(pEagleModel->m_pModelRootObject, true);
	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
	//m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, nAnimationTracks, pEagleModel);
}

CMap::~CMap()
{
}

CModularModel::CModularModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks)
{
	CLoadedModelInfo* pEagleModel = pModel;
	if (!pEagleModel) pEagleModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/ModularModel.bin", NULL);

	SetChild(pEagleModel->m_pModelRootObject, true);
	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, nAnimationTracks, pEagleModel);
}

CModularModel::~CModularModel()
{
}


////////////////////////////////////////////////////////////////////////////////////////////////////
//

CBBObject::CBBObject(int nMeshes) : CGameObject(nMeshes)
{
}

CBBObject::~CBBObject()
{
}

void CBBObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int SharedNum, int nPipelineState)
{
	CGameObject::Render(pd3dCommandList, pCamera, SharedNum, nPipelineState);
}

COtherClientPlayer::COtherClientPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int ClientNum)
{
	for (int anim : MOVE_ANIM) {
		m_moveAnimNum.insert(anim);
	}

	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);
	if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::INGAME)
	{
		if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY)
		{
			if (ClientNum == 3)
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 5, pAngrybotModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);
				m_pSkinnedAnimationController->SetTrackAnimationSet(1, 0);
				m_pSkinnedAnimationController->SetTrackAnimationSet(2, 0);
				m_pSkinnedAnimationController->SetTrackAnimationSet(3, 0);
				m_pSkinnedAnimationController->SetTrackAnimationSet(4, 0);

				for (int i = 1; i < 5; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				for (int i = 1; i < 5; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
			}
			else
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 4, pAngrybotModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, BASIC_ANI::Idle(JOB::ARCHER));
				m_pSkinnedAnimationController->SetTrackAnimationSet(1, BASIC_ANI::Idle(JOB::FIGHTER));
				m_pSkinnedAnimationController->SetTrackAnimationSet(2, BASIC_ANI::Idle(JOB::SWORDMAN));
				m_pSkinnedAnimationController->SetTrackAnimationSet(3, BASIC_ANI::Idle(JOB::WIZARD));

				for (int i = 1; i < 4; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				for (int i = 1; i < 4; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
			}
		}
		else
		{
			m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 5, pAngrybotModel);
			m_pSkinnedAnimationController->SetTrackAnimationSet(0, 12);
			m_pSkinnedAnimationController->SetTrackAnimationSet(1, 13);
			m_pSkinnedAnimationController->SetTrackAnimationSet(2, 14);
			m_pSkinnedAnimationController->SetTrackAnimationSet(3, 15);
			m_pSkinnedAnimationController->SetTrackAnimationSet(4, 16);

			for (int i = 1; i < 5; ++i)
				m_pSkinnedAnimationController->SetTrackEnable(i, false);
			for (int i = 1; i < 5; ++i)
				m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
		}
		

		/*m_pSkinnedAnimationController->SetCallbackKeys(1, 2);

		CAnimationCallbackHandler* pAnimationCallbackHandler = new CSoundCallbackHandler();
		m_pSkinnedAnimationController->SetAnimationCallbackHandler(1, pAnimationCallbackHandler);*/
	}
	else
	{
		if (ClientNum != 3) // When a player is a hero
		{
			int AniTrack = 15;

			m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, AniTrack, pAngrybotModel);
			m_pSkinnedAnimationController->SetTrackAnimationSet(0, BASIC_ANI::Idle(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));
			m_pSkinnedAnimationController->SetTrackAnimationSet(1, BASIC_ANI::WalkForward(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));
			m_pSkinnedAnimationController->SetTrackAnimationSet(2, BASIC_ANI::WalkBack(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));
			m_pSkinnedAnimationController->SetTrackAnimationSet(3, BASIC_ANI::WalkLeft(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));
			m_pSkinnedAnimationController->SetTrackAnimationSet(4, BASIC_ANI::WalkRight(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));

			m_pSkinnedAnimationController->SetTrackAnimationSet(5, BASIC_ANI::Attack(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));	//attack
			m_pSkinnedAnimationController->SetTrackAnimationSet(14, BASIC_ANI::Death(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum])));	//attack
			for (int i = 6; i < 10; ++i)
			{
				int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[ClientNum][i - 6] - PLAYER_SKILL / 4;
				vector<int>	vecSkill = SceneManager::GetInstance()->m_MatchingAniList[skillNum];
				if (vecSkill.size() != 0)
				{
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, vecSkill[0]);
					m_pSkinnedAnimationController->SetDetailSkillAnim_Hero(i, skillNum + 1, vecSkill[0]);
				}				
				else
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, -1);
				if (vecSkill.size() >= 2)
				{
					m_pSkinnedAnimationController->SetTrackContinuousAni(i, true);
					m_pSkinnedAnimationController->SetTrackAnimationSet(i + 4, vecSkill[1]);
					m_pSkinnedAnimationController->SetDetailSkillAnim_Hero(i + 4, skillNum + 1, vecSkill[1]);
				}
			}

			for (int i = 1; i < AniTrack; ++i)
				m_pSkinnedAnimationController->SetTrackEnable(i, false);
			for (int i = 1; i < 5; ++i)
				m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
			if (static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum]) == JOB::FIGHTER)
				m_pSkinnedAnimationController->m_pAnimationTracks[5].SetSpeed(2.f);
			else if (static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[ClientNum]) == JOB::SWORDMAN)
				m_pSkinnedAnimationController->m_pAnimationTracks[5].SetSpeed(2.f);

			m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
			m_pSkinnedAnimationController->m_pAnimationTracks[14].m_nType = ANIMATION_TYPE_ONCE;
			/*m_pSkinnedAnimationController->SetCallbackKeys(1, 2);

			CAnimationCallbackHandler* pAnimationCallbackHandler = new CSoundCallbackHandler();
			m_pSkinnedAnimationController->SetAnimationCallbackHandler(1, pAnimationCallbackHandler);*/
		}
		else
		{
			if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 11, pAngrybotModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, BOSS_OGRE_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(1, BOSS_OGRE_ANI::WalkForward());	//				
				m_pSkinnedAnimationController->SetTrackAnimationSet(2, BOSS_OGRE_ANI::WalkBack());	//				
				m_pSkinnedAnimationController->SetTrackAnimationSet(3, BOSS_OGRE_ANI::WalkLeft());	//				
				m_pSkinnedAnimationController->SetTrackAnimationSet(4, BOSS_OGRE_ANI::WalkRight());	//				
				m_pSkinnedAnimationController->SetTrackAnimationSet(5, BOSS_OGRE_ANI::Attack());
				m_pSkinnedAnimationController->SetTrackAnimationSet(10, BOSS_OGRE_ANI::Death());

				//skill
				for (int i = 6; i < 10; ++i)
				{
					int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][i - 6] - BOSS_SKILL / 2;
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, BOSS_OGRE_ANI::Skill_AniNum(skillNum));
					m_pSkinnedAnimationController->SetDetailSkillAnim_Boss(i, skillNum);
				}

				for (int i = 0; i < 11; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
				m_pSkinnedAnimationController->m_pAnimationTracks[10].m_nType = ANIMATION_TYPE_ONCE;
				for (int i = 1; i < 5; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);

			}
			else if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 10, pAngrybotModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, BOSS_PROGRAMMER_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(1, BOSS_PROGRAMMER_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(2, BOSS_PROGRAMMER_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(3, BOSS_PROGRAMMER_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(4, BOSS_PROGRAMMER_ANI::Idle());	//idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(5, BOSS_PROGRAMMER_ANI::Attack());	//				
				m_pSkinnedAnimationController->SetTrackAnimationSet(10, BOSS_PROGRAMMER_ANI::Death());	//				
				//skill
				for (int i = 6; i < 10; ++i)
				{
					int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][i - 6] - BOSS_SKILL / 2;
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, BOSS_PROGRAMMER_ANI::Skill_AniNum(skillNum));
					m_pSkinnedAnimationController->SetDetailSkillAnim_Boss(i, skillNum);
				}
				for (int i = 0; i < 10; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
				m_pSkinnedAnimationController->m_pAnimationTracks[10].m_nType = ANIMATION_TYPE_ONCE;
					
				for (int i = 1; i < 5; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
			}
			else
				cout << "An error occurs when setting the OtherClients as boss because the job" << endl;
		}
		
	}

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);


	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
		if (ClientNum == 3) {
			if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
			{
				SetScale(OGRE_SCALE, OGRE_SCALE, OGRE_SCALE);
			}
			else if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
			{
				SetScale(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
			}
		}
		else {
			//Hero
			SetScale(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
		}
	}
	else {
		//Lobby
		SetScale(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE);
	}
}

COtherClientPlayer::~COtherClientPlayer()
{

}

void COtherClientPlayer::Update(int id)
{
	if (m_animation < 5 || m_moveAnimNum.contains(m_pSkinnedAnimationController->GetTrackAnimationSet(m_animation))) {//Move, MoveAnim

		XMFLOAT3 targetPosition = XMFLOAT3(NetworkManager::GetInstance()->otherClientsInfo[id].x, NetworkManager::GetInstance()->otherClientsInfo[id].y, NetworkManager::GetInstance()->otherClientsInfo[id].z);
		XMFLOAT3 currentPosition = GetPosition();
		float distance = sqrt(pow(targetPosition.x - currentPosition.x, 2.f) + pow(targetPosition.y - currentPosition.y, 2.f) + pow(targetPosition.z - currentPosition.z, 2.f));

		long long delay = abs(std::chrono::high_resolution_clock::now().time_since_epoch().count() - NetworkManager::GetInstance()->otherClientsInfo[id].lastPacketTime) - std::chrono::nanoseconds(15000000).count();
		if (delay < 0) {
			delay = 0;
		}
		float packetDeltaTime = delay * 0.000001f;

		float weight = NetworkManager::GetInstance()->lerpPercentage;
		int maxDelay = 20.f / weight;

		if (distance < 0.1f || distance > 3.f) {
			SetPosition(targetPosition);
		}
		else {
			XMFLOAT3 previousPosition = XMFLOAT3(NetworkManager::GetInstance()->otherClientsInfo[id].prevX, NetworkManager::GetInstance()->otherClientsInfo[id].prevY, NetworkManager::GetInstance()->otherClientsInfo[id].prevZ);
			XMFLOAT3 predictedPosition = {
				targetPosition.x + (targetPosition.x - previousPosition.x),
				targetPosition.y,
				targetPosition.z + (targetPosition.z - previousPosition.z)
			};

			float lerpPercentage = weight - static_cast<float>(packetDeltaTime) / static_cast<float>(maxDelay);

			if (lerpPercentage < 0.f) {
				lerpPercentage = 1.0f;
			}
			XMFLOAT3 newPosition = {
				currentPosition.x + (predictedPosition.x - currentPosition.x) * lerpPercentage,
				targetPosition.y,
				currentPosition.z + (predictedPosition.z - currentPosition.z) * lerpPercentage
			};

			if (distance)
				SetPosition(newPosition);
		}
	}

	SetLook(NetworkManager::GetInstance()->otherClientsInfo[id].lookX, NetworkManager::GetInstance()->otherClientsInfo[id].lookY, NetworkManager::GetInstance()->otherClientsInfo[id].lookZ);
	SetRight(NetworkManager::GetInstance()->otherClientsInfo[id].rightX, NetworkManager::GetInstance()->otherClientsInfo[id].rightY, NetworkManager::GetInstance()->otherClientsInfo[id].rightZ);
	m_xmf4x4World._21 = 0.f; m_xmf4x4World._22 = 1.f; m_xmf4x4World._23 = 0.f;	//Set Up vector

	if (NetworkManager::GetInstance()->otherClientsInfo[id].skillUsed) {
		UseSkill(NetworkManager::GetInstance()->otherClientsInfo[id].playerSkill, id);
		NetworkManager::GetInstance()->otherClientsInfo[id].skillUsed = false;
	}

	//1:Forward, 2:Back, 3:Left, 4: Right
	if (m_animation < 5)
	{
		switch (NetworkManager::GetInstance()->otherClientsInfo[id].animation)
		{
		case 0:
			SetAnimation(0, 0, 0);
			break;
		case 1:
			SetAnimation(1, 0, 1);
			break;

		case 2:
			SetAnimation(2, 0, 2);
			break;

		case 3:
			SetAnimation(3, 0, 3);
			break;

		case 4:
			SetAnimation(4, 0, 4);
			break;
		default:
			m_pSkinnedAnimationController->SetTrackEnable(m_animation, true);
			break;
		}
	}
	else
	{
		if (m_Ani != ANI_ON_SERVER::NONE)
		{
			if (m_Ani == ANI_ON_SERVER::DEAD)
			{
				int DeathNum = (id != 3) ? 14 : 10;
				SetAnimation(DeathNum, m_animation, DeathNum);
			}
			else if (m_Ani == ANI_ON_SERVER::IDLE)
				SetAnimation(0, m_animation, 0);
		}


		if (m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_nType != ANIMATION_TYPE_LOOP)
		{
			if (m_pSkinnedAnimationController->IsAnimationFinished(m_animation))
			{
				int DeathNum = (id != 3) ? 14 : 10;
				if (m_animation != DeathNum)
				{
					if (m_pSkinnedAnimationController->GetTrackContinuousAni(m_animation))
					{
						m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
						m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
						m_pSkinnedAnimationController->SetTrackEnable(m_animation + 4, true);
						m_pSkinnedAnimationController->CheckingAniChage();
						if (m_pSkinnedAnimationController->m_bAniChange)
						{
							m_pSkinnedAnimationController->m_bBlend = true;
							m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
							m_pSkinnedAnimationController->m_iPostTrackNum = m_animation + 4;
						}
						m_animation = m_animation + 4;
					}
					else
					{
						m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
						m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
						m_pSkinnedAnimationController->SetTrackEnable(0, true);
						m_pSkinnedAnimationController->CheckingAniChage();
						if (m_pSkinnedAnimationController->m_bAniChange)
						{
							m_pSkinnedAnimationController->m_bBlend = true;
							m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
							m_pSkinnedAnimationController->m_iPostTrackNum = 0;
						}
						m_animation = 0;
					}
				}
			}
		}
		else
		{
			if (m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_bLoop == false)
			{
				m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
				m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
				m_pSkinnedAnimationController->SetTrackEnable(0, true);
				m_pSkinnedAnimationController->CheckingAniChage();
				if (m_pSkinnedAnimationController->m_bAniChange)
				{
					m_pSkinnedAnimationController->m_bBlend = true;
					m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
					m_pSkinnedAnimationController->m_iPostTrackNum = 0;
				}
				m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_bLoop = true;
				m_animation = 0;
			}
			else
			{
				if (m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_bOnOff == true)
				{
					if (m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_bOnOffToggle == false)
					{
						m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
						m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
						m_pSkinnedAnimationController->SetTrackEnable(0, true);
						m_pSkinnedAnimationController->CheckingAniChage();
						if (m_pSkinnedAnimationController->m_bAniChange)
						{
							m_pSkinnedAnimationController->m_bBlend = true;
							m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
							m_pSkinnedAnimationController->m_iPostTrackNum = 0;
						}
						m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_fAnimLoopTime = 0.f;
						m_pSkinnedAnimationController->m_pAnimationTracks[m_animation].m_iAnimLoopCount = 0;

						m_animation = 0;
					}
				}
			}
		}
	}
	
}

void COtherClientPlayer::SetLook(float x, float y, float z)
{
	m_xmf4x4ToParent._31 = x; m_xmf4x4ToParent._32 = y; m_xmf4x4ToParent._33 = z;
}

void COtherClientPlayer::SetRight(float x, float y, float z)
{
	m_xmf4x4ToParent._11 = x; m_xmf4x4ToParent._12 = y; m_xmf4x4ToParent._13 = z;
}


void COtherClientPlayer::UseSkill(SKILLKIND nSkillNum, int id)
{
	for (auto p : SceneManager::GetInstance()->m_ParticleInfo[static_cast<PARTICLE_SITUATION>(id)][static_cast<int>(nSkillNum) - 1])
	{
		if (!p->show)
		{
			p->show = true;
			p->Dir = XMFLOAT3(m_xmf4x4ToParent._31, m_xmf4x4ToParent._32, m_xmf4x4ToParent._33);
			p->pos = GetPosition();
		}
	}

	if (m_pSkinnedAnimationController->GetTrackAnimationSet(static_cast<int>(nSkillNum) + 4) != -1)
	{
		if (m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOff)
		{
			m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle = true;
			cout << "Otherclient input Toggle Skill" << endl;
		}

		if (m_animation < 5) {
			m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
			m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
			m_pSkinnedAnimationController->SetTrackEnable(static_cast<int>(nSkillNum) + 4, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = 0;
				m_pSkinnedAnimationController->m_iPostTrackNum = static_cast<int>(nSkillNum) + 4;
			}
			m_animation = static_cast<int>(nSkillNum) + 4;

			if (sqrtf(powf(NetworkManager::GetInstance()->myInfo->x - NetworkManager::GetInstance()->otherClientsInfo[id].x, 2.f) + powf(NetworkManager::GetInstance()->myInfo->z - NetworkManager::GetInstance()->otherClientsInfo[id].z, 2.f)) < SOUND_DISTANCE) {

				if (nSkillNum == SKILLKIND::LEFTCLICK) {
					if (id != 3)
						SoundManager::GetInstance()->Play_Sound(NetworkManager::GetInstance()->readySceneInfo->playerJobs[id], CHANNELID::PLAYER);
					else if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[id] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE)) {
						SoundManager::GetInstance()->Play_Sound(static_cast<int>(BOSSJOB::OGRE), CHANNELID::PLAYER);
					}
				}
				else {
					SoundManager::GetInstance()->Play_Sound(NetworkManager::GetInstance()->readySceneInfo->selectSkills[id][static_cast<int>(nSkillNum) - 2], CHANNELID::PLAYER);
				}
			}
		}
	}	
}

void COtherClientPlayer::SetAnimation(int AniNum, int preTrackNum, int postTrackNum)
{
	m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
	m_pSkinnedAnimationController->SetTrackEnable(AniNum, true);
	m_pSkinnedAnimationController->CheckingAniChage();
	if (m_pSkinnedAnimationController->m_bAniChange)
	{
		m_pSkinnedAnimationController->m_bBlend = true;
		m_pSkinnedAnimationController->m_iPreTrackNum = preTrackNum;
		m_pSkinnedAnimationController->m_iPostTrackNum = postTrackNum;
	}
	m_animation = AniNum;
	if (m_Ani != ANI_ON_SERVER::NONE)
		m_Ani = ANI_ON_SERVER::NONE;
}

void COtherClientPlayer::SetReadyAnim(int AniNum)
{
	if (m_pSkinnedAnimationController)
	{
		m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
		m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
		m_pSkinnedAnimationController->SetTrackEnable(AniNum, true);
		m_animation = AniNum;
	}
}

void COtherClientPlayer::SetIdleAnim()
{
	if (m_pSkinnedAnimationController)
	{
		m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
		m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
		m_pSkinnedAnimationController->SetTrackEnable(0, true);
		m_animation = 0;
	}
}

CNpc::CNpc()
{
}

CNpc::CNpc(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int type)
{
}

CNpc::~CNpc()
{
}

void CNpc::Update(int id)
{
}

void CNpc::SetAnimation(int anim)
{
	if (anim == m_animation)
		return;

	if (m_pSkinnedAnimationController) {
		m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
		m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
		m_pSkinnedAnimationController->SetTrackEnable(anim, true);
		m_pSkinnedAnimationController->CheckingAniChage();
		if (m_pSkinnedAnimationController->m_bAniChange)
		{
			m_pSkinnedAnimationController->m_bBlend = true;
			m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
			m_pSkinnedAnimationController->m_iPostTrackNum = anim;
		}
		m_animation = anim;
	}

	if (m_Ani != ANI_ON_SERVER::NONE)
		m_Ani = ANI_ON_SERVER::NONE;

}

void CNpc::SetLook(float x, float y, float z)
{
	m_xmf4x4ToParent._31 = x; m_xmf4x4ToParent._32 = y; m_xmf4x4ToParent._33 = z;
}

void CNpc::SetRight(float x, float y, float z)
{
	m_xmf4x4ToParent._11 = x; m_xmf4x4ToParent._12 = y; m_xmf4x4ToParent._13 = z;
}

CMonster::CMonster(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CMonster::~CMonster()
{
}

void CMonster::Update(int id)
{
	SetPosition(NetworkManager::GetInstance()->monsterInfo[id].x, NetworkManager::GetInstance()->monsterInfo[id].y, NetworkManager::GetInstance()->monsterInfo[id].z);
	SetLook(NetworkManager::GetInstance()->monsterInfo[id].lookX, NetworkManager::GetInstance()->monsterInfo[id].lookY, NetworkManager::GetInstance()->monsterInfo[id].lookZ);
	SetRight(NetworkManager::GetInstance()->monsterInfo[id].rightX, NetworkManager::GetInstance()->monsterInfo[id].rightY, NetworkManager::GetInstance()->monsterInfo[id].rightZ);
	m_xmf4x4World._21 = 0.f; m_xmf4x4World._22 = 1.f; m_xmf4x4World._23 = 0.f;	//Set Up vector

	if (m_Ani != ANI_ON_SERVER::NONE)
	{
		if (m_Ani == ANI_ON_SERVER::DEAD)
		{
			SetAnimation(GetDeathAnim());
		}
		else if (m_Ani == ANI_ON_SERVER::IDLE)
		{
			SetAnimation(0);//idle
		}
	}

	if (m_pSkinnedAnimationController->IsAnimationFinished(m_animation)) {
		if (m_animation != m_deathAnim)
		{
			m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
			m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
			m_pSkinnedAnimationController->SetTrackEnable(0, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
				m_pSkinnedAnimationController->m_iPostTrackNum = 0;
			}
			if (m_animation == m_attackAnim) {
				NetworkManager::GetInstance()->SendNpcAttackFinishPacket(id + MAX_MINION);
			}
			m_animation = 0;
		}
	}
}

CUniqueRed::CUniqueRed(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 10, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//Walk
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//Run
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//Basic Attack
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//Claw Attack
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//Flame Attack
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Sleep
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Get Hit
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//Die

	for (int i = 1; i < 10; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 4; i < 7; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}
	m_pSkinnedAnimationController->m_pAnimationTracks[8].m_nType = ANIMATION_TYPE_ONCE;
	m_pSkinnedAnimationController->m_pAnimationTracks[9].m_nType = ANIMATION_TYPE_ONCE;

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 2;
	m_runAnim = 3;
	m_attackAnim = 4;
	m_deathAnim = 9;

	m_attackSoundString = L"RedAttack.wav";
	m_deathSoundString = L"RedDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CRareGreen::CRareGreen(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 10, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//Walk
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//Run
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//Basic Attack
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//Fireball Shoot
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//Scream
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//Tail Attack
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Get Hit
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Defend
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//Die

	for (int i = 1; i < 10; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 3; i < 10; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	m_walkAnim = 1;
	m_runAnim = 2;
	m_attackAnim = 3;
	m_deathAnim = 9;

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_attackSoundString = L"GreenAttack.wav";
	m_deathSoundString = L"GreenDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);

	SetScale(RARE_GREEN_SCALE, RARE_GREEN_SCALE, RARE_GREEN_SCALE);
}

CRareGolem::CRareGolem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 6, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//Walk
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//Attack01
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//Attack02
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//Get Hit
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//Die

	for (int i = 1; i < 6; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 2; i < 6; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	m_pSkinnedAnimationController->m_pAnimationTracks[2].SetSpeed(2.f);
	m_pSkinnedAnimationController->m_pAnimationTracks[3].SetSpeed(2.f);

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 1;
	m_runAnim = 1;
	m_attackAnim = 2;
	m_deathAnim = 5;

	m_attackSoundString = L"GolemAttack.wav";
	m_deathSoundString = L"GolemDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CNormalBear::CNormalBear(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 16, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	// Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//Idle_Combat
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//WalkForward
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//WalkBackward
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//RunForward
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//RunBackward
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//Attack1
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Attack2
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Attack3
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//Attack4
	m_pSkinnedAnimationController->SetTrackAnimationSet(10, 10);	//Eat
	m_pSkinnedAnimationController->SetTrackAnimationSet(11, 11);	//Buff
	m_pSkinnedAnimationController->SetTrackAnimationSet(12, 12);	//GetHit
	m_pSkinnedAnimationController->SetTrackAnimationSet(13, 13);	//Death
	m_pSkinnedAnimationController->SetTrackAnimationSet(14, 14);	//Sleep
	m_pSkinnedAnimationController->SetTrackAnimationSet(15, 15);	//Sit

	for (int i = 1; i < 16; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 6; i < 14; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 2;
	m_runAnim = 4;
	m_attackAnim = 6;
	m_deathAnim = 13;

	m_attackSoundString = L"BearAttack.wav";
	m_deathSoundString = L"BearDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CNormalMinotaur::CNormalMinotaur(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 13, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//Idle
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//WalkForward
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//WalkForward2
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//WalkBackward
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//Run
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//Attack1
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//Attack2
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Attack3
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Attack4
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//Attack5
	m_pSkinnedAnimationController->SetTrackAnimationSet(10, 10);	//hit1
	m_pSkinnedAnimationController->SetTrackAnimationSet(11, 11);	//hit2
	m_pSkinnedAnimationController->SetTrackAnimationSet(12, 12);	//death	

	for (int i = 1; i < 13; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 5; i < 13; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 1;
	m_runAnim = 4;
	m_attackAnim = 5;
	m_deathAnim = 12;

	m_attackSoundString = L"MinotaurAttack.wav";
	m_deathSoundString = L"MinotaurDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CNormalChest::CNormalChest(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 11, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//IdleNormal
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//IdleBattle
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//Run
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//WalkForward
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//WalkLeft
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//WalkRight
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//WalkBackward
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Attack01
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Attack02
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//GetHit
	m_pSkinnedAnimationController->SetTrackAnimationSet(10, 10);	//Death

	for (int i = 1; i < 11; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 7; i < 11; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 3;
	m_runAnim = 2;
	m_attackAnim = 7;
	m_deathAnim = 10;

	m_attackSoundString = L"ChestAttack.wav";
	m_deathSoundString = L"ChestDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CNormalBeholder::CNormalBeholder(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 13, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//IdleNormal
	m_pSkinnedAnimationController->SetTrackAnimationSet(1, 1);	//IdleBattle
	m_pSkinnedAnimationController->SetTrackAnimationSet(2, 2);	//WalkForward
	m_pSkinnedAnimationController->SetTrackAnimationSet(3, 3);	//WalkLeft
	m_pSkinnedAnimationController->SetTrackAnimationSet(4, 4);	//WalkRight
	m_pSkinnedAnimationController->SetTrackAnimationSet(5, 5);	//WalkBackward
	m_pSkinnedAnimationController->SetTrackAnimationSet(6, 6);	//Attack01
	m_pSkinnedAnimationController->SetTrackAnimationSet(7, 7);	//Attack02 Spell
	m_pSkinnedAnimationController->SetTrackAnimationSet(8, 8);	//Attack02 Repeat
	m_pSkinnedAnimationController->SetTrackAnimationSet(9, 9);	//Attack03
	m_pSkinnedAnimationController->SetTrackAnimationSet(10, 10);	//Run
	m_pSkinnedAnimationController->SetTrackAnimationSet(11, 11);	//GetHit
	m_pSkinnedAnimationController->SetTrackAnimationSet(12, 12);	//Death

	for (int i = 1; i < 13; ++i)
		m_pSkinnedAnimationController->SetTrackEnable(i, false);

	for (int i = 6; i < 10; ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}
	m_pSkinnedAnimationController->m_pAnimationTracks[11].m_nType = ANIMATION_TYPE_ONCE;
	m_pSkinnedAnimationController->m_pAnimationTracks[12].m_nType = ANIMATION_TYPE_ONCE;

	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	m_walkAnim = 2;
	m_runAnim = 10;
	m_attackAnim = 6;
	m_deathAnim = 12;

	m_attackSoundString = L"BeholderAttack.wav";
	m_deathSoundString = L"BeholderDeath.wav";

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CMinion::CMinion(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int type)
{
	CLoadedModelInfo* pAngrybotModel = pModel;

	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, static_cast<int>(MINION_ANIM::COUNT), pAngrybotModel);
	for (int i = 0; i < static_cast<int>(MINION_ANIM::COUNT); ++i) {
		m_pSkinnedAnimationController->SetTrackAnimationSet(i, i);
	}

	for (int i = 0; i < static_cast<int>(MINION_ANIM::COUNT); ++i) {
		m_pSkinnedAnimationController->SetTrackEnable(i, false);
	}
	for (int i = static_cast<int>(MINION_ANIM::HIT); i < static_cast<int>(MINION_ANIM::COUNT); ++i) {
		m_pSkinnedAnimationController->m_pAnimationTracks[i].m_nType = ANIMATION_TYPE_ONCE;
	}

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CMinion::~CMinion()
{
}

void CMinion::Update(int id)
{
	SetPosition(NetworkManager::GetInstance()->npcInfo[id].x, NetworkManager::GetInstance()->npcInfo[id].y, NetworkManager::GetInstance()->npcInfo[id].z);
	SetLook(NetworkManager::GetInstance()->npcInfo[id].lookX, NetworkManager::GetInstance()->npcInfo[id].lookY, NetworkManager::GetInstance()->npcInfo[id].lookZ);
	SetRight(NetworkManager::GetInstance()->npcInfo[id].rightX, NetworkManager::GetInstance()->npcInfo[id].rightY, NetworkManager::GetInstance()->npcInfo[id].rightZ);
	m_xmf4x4World._21 = 0.f; m_xmf4x4World._22 = 1.f; m_xmf4x4World._23 = 0.f;	//Set Up vector

	if (m_Ani != ANI_ON_SERVER::NONE)
	{
		if (m_Ani == ANI_ON_SERVER::DEAD)
		{
			SetAnimation(static_cast<int>(MINION_ANIM::DIE));
		}
		else if (m_Ani == ANI_ON_SERVER::IDLE)
		{
			SetAnimation(static_cast<int>(MINION_ANIM::WALK));
		}
	}


	if (m_pSkinnedAnimationController->IsAnimationFinished(m_animation)) {
		if (m_animation != static_cast<int>(MINION_ANIM::DIE))
		{
			m_pSkinnedAnimationController->SetTrackEnable(m_animation, false);
			m_pSkinnedAnimationController->SetTrackPosition(m_animation, 0.0f);
			m_pSkinnedAnimationController->SetTrackEnable(0, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = m_animation;
				m_pSkinnedAnimationController->m_iPostTrackNum = 0;
			}
			if (m_animation == static_cast<int>(MINION_ANIM::ATTACK1) || m_animation == static_cast<int>(MINION_ANIM::ATTACK2)) {
				NetworkManager::GetInstance()->SendNpcAttackFinishPacket(id);
			}
			m_animation = 0;
		}
	}
}

CTowerAttack::CTowerAttack(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	if (!pAngrybotModel)
		pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Cube.bin", NULL);

	SetChild(pAngrybotModel->m_pModelRootObject, true);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CTowerAttack::~CTowerAttack()
{
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CBlendObject::CBlendObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks)
{
	CLoadedModelInfo* pEagleModel = pModel;
	if (!pEagleModel) pEagleModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/BlendObjectsForLobby2.bin", NULL);

	SetChild(pEagleModel->m_pModelRootObject, true);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CBlendObject::~CBlendObject()
{
}

void CPlayerObject::Customize(ModelCustomize customization)
{
	DrawOff();
	m_bIsRender = true;
	m_CustomizeInfo = customization;
	char frameName[128];
	for (int i = 1; i < 28; i++) {
		int value = *((short*)&customization + i);
#ifdef WITH_DATABASE
		if(value != -1) {
#else
		if (value != 0) {
#endif
			if (i < 15) {
				sprintf(frameName, "%s_%02d", FrameNames[i], value);
			}
			else {
				const char* gender = customization.Chr_Sex == 0 ? "Male" : "Female";
				sprintf(frameName, "%s_%s_%02d", FrameNames[i], gender, value);
			}
			CGameObject* a = FindFrame(frameName);
			if (a)
				a->m_bIsRender = true;
		}
	}

}

void CPlayerObject::SetWeapon(JOB playerJob)
{
	auto shield = FindFrame("SM_Wep_Shield_14");
	auto bow = FindFrame("SM_bow_creep");
	auto sword = FindFrame("SM_Wep_Sword_01");
	auto staff = FindFrame("SM_Wep_Staff_02");


	shield->m_bIsRender = false;
	bow->m_bIsRender = false;
	sword->m_bIsRender = false;
	staff->m_bIsRender = false;

	switch (playerJob)
	{
	case JOB::ARCHER:
		bow->m_bIsRender = true;
		break;
	case JOB::FIGHTER:

		break;
	case JOB::WIZARD:
		staff->m_bIsRender = true;
		break;
	case JOB::SWORDMAN:
		sword->m_bIsRender = true;
		shield->m_bIsRender = true;
		break;
	default:
		break;
	}
}

void CPlayerObject::ModifyModel()
{
	const int Chr_HeadCoverings_Base_Hair = 11;						//0
	const int Chr_HeadCoverings_No_FacialHair = 4;					//1
	const int Chr_HeadCoverings_No_Hair = 13;						//2
	const int Chr_Hair = 38;										//3
	const int Chr_HelmetAttachment = 13;							//4
	const int Chr_BackAttachment = 15;								//5
	const int Chr_ShoulderAttachRight = 21;							//6
	const int Chr_ShoulderAttachLeft = 21;							//7
	const int Chr_ElbowAttachRight = 6;								//8
	const int Chr_ElbowAttachLeft = 6;								//9
	const int Chr_HipsAttachment = 12;								//0
	const int Chr_KneeAttachRight = 11;								//10
	const int Chr_KneeAttachLeft = 11;								//11
	const int Chr_Ear_Ear = 3;										//12
																	//
	const int Chr_Head = 22;										//13
	const int Chr_Head_No_Elements = 13;							//14
	const int Chr_Eyebrow = m_CustomizeInfo.Chr_Sex == 1 ? 7 : 10;	//15
	const int Chr_Torso = 28;										//16
	const int Chr_ArmUpperRight = 20;								//17
	const int Chr_ArmUpperLeft = 20;								//18
	const int Chr_ArmLowerRight = 18;								//19
	const int Chr_ArmLowerLeft = 18;								//20
	const int Chr_HandRight = 17;									//21
	const int Chr_HandLeft = 17;									//22
	const int Chr_Hips = 28;										//23
	const int Chr_LegRight = 19;									//24
	const int Chr_LegLeft = 19;										//25

	if (m_CustomizeInfo.Chr_HeadCoverings_Base_Hair < Chr_HeadCoverings_Base_Hair)m_CustomizeInfo.Chr_HeadCoverings_Base_Hair++;
	else m_CustomizeInfo.Chr_HeadCoverings_Base_Hair = 1;

	if (m_CustomizeInfo.Chr_HeadCoverings_No_FacialHair < Chr_HeadCoverings_No_FacialHair)m_CustomizeInfo.Chr_HeadCoverings_No_FacialHair++;
	else m_CustomizeInfo.Chr_HeadCoverings_No_FacialHair = 1;

	if (m_CustomizeInfo.Chr_HeadCoverings_No_Hair < Chr_HeadCoverings_No_Hair)m_CustomizeInfo.Chr_HeadCoverings_No_Hair++;
	else m_CustomizeInfo.Chr_HeadCoverings_No_Hair = 1;

	if (m_CustomizeInfo.Chr_Hair < Chr_Hair)m_CustomizeInfo.Chr_Hair++;
	else m_CustomizeInfo.Chr_Hair = 1;

	if (m_CustomizeInfo.Chr_HelmetAttachment < Chr_HelmetAttachment)m_CustomizeInfo.Chr_HelmetAttachment++;
	else m_CustomizeInfo.Chr_HelmetAttachment = 1;

	if (m_CustomizeInfo.Chr_ShoulderAttachRight < Chr_ShoulderAttachRight)m_CustomizeInfo.Chr_ShoulderAttachRight++;
	else m_CustomizeInfo.Chr_ShoulderAttachRight = 1;

	if (m_CustomizeInfo.Chr_ShoulderAttachLeft < Chr_ShoulderAttachLeft)m_CustomizeInfo.Chr_ShoulderAttachLeft++;
	else m_CustomizeInfo.Chr_ShoulderAttachLeft = 1;

	if (m_CustomizeInfo.Chr_ElbowAttachRight < Chr_ElbowAttachRight)m_CustomizeInfo.Chr_ElbowAttachRight++;
	else m_CustomizeInfo.Chr_ElbowAttachRight = 1;

	if (m_CustomizeInfo.Chr_ElbowAttachLeft < Chr_ElbowAttachLeft)m_CustomizeInfo.Chr_ElbowAttachLeft++;
	else m_CustomizeInfo.Chr_ElbowAttachLeft = 1;

	if (m_CustomizeInfo.Chr_HipsAttachment < Chr_HipsAttachment)m_CustomizeInfo.Chr_HipsAttachment++;
	else m_CustomizeInfo.Chr_HipsAttachment = 1;

	if (m_CustomizeInfo.Chr_KneeAttachRight < Chr_KneeAttachRight)m_CustomizeInfo.Chr_KneeAttachRight++;
	else m_CustomizeInfo.Chr_KneeAttachRight = 1;

	if (m_CustomizeInfo.Chr_KneeAttachLeft < Chr_KneeAttachLeft)m_CustomizeInfo.Chr_KneeAttachLeft++;
	else m_CustomizeInfo.Chr_KneeAttachLeft = 1;

	if (m_CustomizeInfo.Chr_Ear_Ear < Chr_Ear_Ear)m_CustomizeInfo.Chr_Ear_Ear++;
	else m_CustomizeInfo.Chr_Ear_Ear = 1;

	if (m_CustomizeInfo.Chr_Head < Chr_Head)m_CustomizeInfo.Chr_Head++;
	else m_CustomizeInfo.Chr_Head = 1;

	if (m_CustomizeInfo.Chr_Head_No_Elements < Chr_Head_No_Elements)m_CustomizeInfo.Chr_Head_No_Elements++;
	else m_CustomizeInfo.Chr_Head_No_Elements = 1;

	if (m_CustomizeInfo.Chr_Eyebrow < Chr_Eyebrow)m_CustomizeInfo.Chr_Eyebrow++;
	else m_CustomizeInfo.Chr_Eyebrow = 1;

	if (m_CustomizeInfo.Chr_Torso < Chr_Torso)m_CustomizeInfo.Chr_Torso++;
	else m_CustomizeInfo.Chr_Torso = 1;

	if (m_CustomizeInfo.Chr_ArmUpperRight < Chr_ArmUpperRight)m_CustomizeInfo.Chr_ArmUpperRight++;
	else m_CustomizeInfo.Chr_ArmUpperRight = 1;

	if (m_CustomizeInfo.Chr_ArmUpperLeft < Chr_ArmUpperLeft)m_CustomizeInfo.Chr_ArmUpperLeft++;
	else m_CustomizeInfo.Chr_ArmUpperLeft = 1;

	if (m_CustomizeInfo.Chr_ArmLowerRight < Chr_ArmLowerRight)m_CustomizeInfo.Chr_ArmLowerRight++;
	else m_CustomizeInfo.Chr_ArmLowerRight = 1;

	if (m_CustomizeInfo.Chr_ArmLowerLeft < Chr_ArmLowerLeft)m_CustomizeInfo.Chr_ArmLowerLeft++;
	else m_CustomizeInfo.Chr_ArmLowerLeft = 1;

	if (m_CustomizeInfo.Chr_BackAttachment < Chr_BackAttachment)m_CustomizeInfo.Chr_BackAttachment++;
	else m_CustomizeInfo.Chr_BackAttachment = 1;

	if (m_CustomizeInfo.Chr_HandRight < Chr_HandRight)m_CustomizeInfo.Chr_HandRight++;
	else m_CustomizeInfo.Chr_HandRight = 1;

	if (m_CustomizeInfo.Chr_HandLeft < Chr_HandLeft)m_CustomizeInfo.Chr_HandLeft++;
	else m_CustomizeInfo.Chr_HandLeft = 1;

	if (m_CustomizeInfo.Chr_Hips < Chr_Hips)m_CustomizeInfo.Chr_Hips++;
	else m_CustomizeInfo.Chr_Hips = 1;

	if (m_CustomizeInfo.Chr_LegRight < Chr_BackAttachment)m_CustomizeInfo.Chr_LegRight++;
	else m_CustomizeInfo.Chr_LegRight = 1;

	if (m_CustomizeInfo.Chr_LegLeft < Chr_LegLeft)m_CustomizeInfo.Chr_LegLeft++;
	else m_CustomizeInfo.Chr_LegLeft = 1;

	Customize(m_CustomizeInfo);
}

void CPlayerObject::ChangeSex()
{
	m_CustomizeInfo.Chr_Sex = m_CustomizeInfo.Chr_Sex == 0 ? 1 : 0;
	if (m_CustomizeInfo.Chr_Sex && m_CustomizeInfo.Chr_Eyebrow > 7)
		m_CustomizeInfo.Chr_Eyebrow = 7;
	Customize(m_CustomizeInfo);
}

CSkillObject::CSkillObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel)
{
	SetChild(pModel->m_pModelRootObject, true);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CSkillObject::Update(const XMFLOAT3& pos, const XMFLOAT3& look)
{
	XMFLOAT3 right = Vector3::CrossProduct(look, XMFLOAT3(0.f, 1.0f, 0.f), true);

	m_xmf4x4ToParent._11 = right.x; m_xmf4x4ToParent._12 = right.y; m_xmf4x4ToParent._13 = right.z;
	m_xmf4x4ToParent._21 = 0.f; m_xmf4x4ToParent._22 = 1.f; m_xmf4x4ToParent._23 = 0.f;
	m_xmf4x4ToParent._31 = look.x; m_xmf4x4ToParent._32 = look.y; m_xmf4x4ToParent._33 = look.z;

	SetScale(m_scaleValue.x, m_scaleValue.y, m_scaleValue.z);
	SetPosition(pos);	

	//UpdateTransform(NULL);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CUIObject::CUIObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CTexture* texture, XMFLOAT2 meshRectSize, XMFLOAT2 screenPos, TEXTURETYPE type, float val, XMFLOAT2 uvOffset) : CGameObject(1)
{
	float width = meshRectSize.x;
	float height = meshRectSize.y;
	CTexturedRectMesh* pTextureRectMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, width, height, 0.0f, 0.0f, 0.0f, 0.0f);
	CMaterial* pTextureMaterial = new CMaterial(1);


	collisionBox = CollisionBox(screenPos.x, screenPos.y, width, height);
	pTextureRectMesh->SetType(type);
	pTextureRectMesh->SetValue(val);
	pTextureRectMesh->SetUV(uvOffset);
	pTextureRectMesh->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pTextureMaterial->SetTexture(texture);
	SetMesh(pTextureRectMesh);
	SetMaterial(0, pTextureMaterial);
	SetObjectType(OBJ_TYPE::TEXTURE);
	SetScreenPosition(screenPos);


	// Initialize Callback Functions. If no mouse events, do not initialize.
	switch (type)
	{
	case TEXTURETYPE::BUTTON:
	{
		SetOnClickCallback([this]() {this->m_pMesh->SetValue(0.5f); });
		SetOnHoverCallback([this]() {this->m_pMesh->SetValue(1.5f); });
		SetOnHoverEndCallback([this]() {this->m_pMesh->SetValue(1.0f); });
		SetOnReleaseCallback([this]() {this->m_pMesh->SetValue(1.0f); });
	}
	break;

	default:
		break;
	}
	
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CUIObject::~CUIObject()
{
}

void CUIObject::OnClick()
{
	if (onClickCallback && objState == UIOBJECTSTATE::HOVER) {
		onClickCallback();
		objState = UIOBJECTSTATE::CLICK;
		SoundManager::GetInstance()->Play_Sound(L"mouseClick.wav", CHANNELID::EFFECT, 0.3f);
	}
}

void CUIObject::OnHover()
{
	if (onHoverCallback) {
		onHoverCallback();
		if (objState != UIOBJECTSTATE::HOVER)
			SoundManager::GetInstance()->Play_Sound(L"mouseHover.wav", CHANNELID::EFFECT, 0.4f);
		objState = UIOBJECTSTATE::HOVER;
	}
}

void CUIObject::OnHoverEnd()
{
	if (onHoverEndCallback) {
		onHoverEndCallback();
		objState = UIOBJECTSTATE::DEFAULT;
	}
}

CUIObject* CUIObject::OnRelease()
{
	if (onReleaseCallback && objState == UIOBJECTSTATE::CLICK) {
		onReleaseCallback();
		objState = UIOBJECTSTATE::DEFAULT;
	}
	return nullptr;
}

CUIObject* CUIObject::OnMouseMoved(float mouseX, float mouseY)
{
	if (onHoverCallback) {
		if (mouseX >= collisionBox.left && mouseX <= collisionBox.right &&
			mouseY >= collisionBox.top && mouseY <= collisionBox.bottom) {
			OnHover();
			return this;
		}
		else
		{
			OnHoverEnd();
			if (objState == UIOBJECTSTATE::HOVER)
				return nullptr;
			else if (objState == UIOBJECTSTATE::CLICK)
				return this;
		}
	}
	return nullptr;
}

void CUIObject::SetBasicButtonEvents()
{
	SetOnClickCallback([this]() {this->m_pMesh->SetValue(0.5f); });
	SetOnHoverCallback([this]() {this->m_pMesh->SetValue(1.5f); });
	SetOnHoverEndCallback([this]() {this->m_pMesh->SetValue(1.0f); });
	SetOnReleaseCallback([this]() {this->m_pMesh->SetValue(1.0f); });
}

void CUIObject::SetScreenPosition(XMFLOAT2 xmf2Position)
{
	CGameObject::SetScreenPosition(xmf2Position);

	float width = collisionBox.right - collisionBox.left;
	float height = collisionBox.bottom - collisionBox.top;
	XMFLOAT2 pos = GetScreenPosition();
	collisionBox = CollisionBox(pos.x, pos.y, width, height);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CParticleObject::CParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CTexture* Texture, CTexture* RandowmValueTexture, CTexture* RandowmValueSphereTexture, CShader* pShader, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Velocity, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles, UINT nType) : CGameObject(1)
{
	CParticleMesh* pMesh = new CParticleMesh(pd3dDevice, pd3dCommandList, xmf3Position, xmf3Velocity, fLifetime, xmf3Acceleration, xmf3Color, xmf2Size, nMaxParticles, nType);
	SetMesh(pMesh);
	
	CMaterial* pMaterial = new CMaterial(1);
	pMaterial->SetTexture(Texture);
	Texture->AddRef();

	//	m_pRandowmValueTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);
	m_pRandowmValueTexture = RandowmValueTexture;
	m_pRandowmValueTexture->AddRef();
	m_pRandowmValueOnSphereTexture = RandowmValueSphereTexture;
	m_pRandowmValueOnSphereTexture->AddRef();

	
	pShader->AddRef();

	pMaterial->SetShader(pShader);
	SetMaterial(0, pMaterial);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CParticleObject::~CParticleObject()
{
	if (m_pRandowmValueTexture) m_pRandowmValueTexture->Release();
	if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->Release();

	ReleaseShaderVariables();
}

void CParticleObject::ReleaseUploadBuffers()
{
	if (m_pRandowmValueTexture) m_pRandowmValueTexture->ReleaseUploadBuffers();
	if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->ReleaseUploadBuffers();

	CGameObject::ReleaseUploadBuffers();
}

void CParticleObject::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dcbForwardGpuVirtualAddress = m_pd3dcbParticle->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(25, d3dcbForwardGpuVirtualAddress);

	::memcpy(&m_pcbMappedParticle->m_xmForwardVector, &m_xmForwardVector, sizeof(XMFLOAT3));
	::memcpy(&m_pcbMappedParticle->m_xm4Color, &m_xm4Color, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedParticle->m_fSize, &m_fSize, sizeof(float));
	::memcpy(&m_pcbMappedParticle->m_fLifeTime, &m_fLifeTime, sizeof(float));
	::memcpy(&m_pcbMappedParticle->m_iParticleNum, &m_iParticleNum, sizeof(int));
	::memcpy(&m_pcbMappedParticle->m_iTotalSpriteNum, &m_iTotalSpriteNum, sizeof(int));
	::memcpy(&m_pcbMappedParticle->m_iWidthSpriteNum, &m_iWidthSpriteNum, sizeof(int));
	::memcpy(&m_pcbMappedParticle->m_iCurrentSpriteNum, &m_iCurrentSpriteNum, sizeof(int));
	::memcpy(&m_pcbMappedParticle->m_iboolLean, &m_iboolLean, sizeof(int));
}

void CParticleObject::AllCreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
	CGameObject::AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
	UINT ncbElementBytes = ((sizeof(PARTICLE_INFO) + 255) & ~255); //256의 배수
	m_pd3dcbParticle = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbParticle->Map(0, NULL, (void**)&m_pcbMappedParticle);


}

void CParticleObject::ReleaseShaderVariables()
{
	if (m_pd3dcbParticle)
	{
		m_pd3dcbParticle->Unmap(0, NULL);
		m_pd3dcbParticle->Release();
		m_pd3dcbParticle = NULL;
	}
}

void CParticleObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_ppMaterials[0])
	{

		if (m_ppMaterials[0]->m_pShader) m_ppMaterials[0]->m_pShader->OnPrepareRender(pd3dCommandList, 0);
		m_ppMaterials[0]->UpdateShaderVariable(pd3dCommandList);
		//if (m_ppMaterials[0]->m_pTexture) m_ppMaterials[0]->m_pTexture->UpdateShaderVariables(pd3dCommandList);

		if (m_pRandowmValueTexture) m_pRandowmValueTexture->UpdateShaderVariables(pd3dCommandList);
		if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->UpdateShaderVariables(pd3dCommandList);
	}

	//UpdateShaderVariables(pd3dCommandList);
	UpdateShaderVariable(pd3dCommandList, &m_xmf4x4World);
	UpdateShaderVariables(pd3dCommandList);

	if (m_pMesh)
	{
		reinterpret_cast<CParticleMesh*>(m_pMesh)->PreRender(pd3dCommandList, 0); //Stream Output
		m_pMesh->Render(pd3dCommandList, 0); //Stream Output
		//m_pMesh->PostRender(pd3dCommandList, 0); //Stream Output
	}

	if (m_ppMaterials[0] && m_ppMaterials[0]->m_pShader) m_ppMaterials[0]->m_pShader->OnPrepareRender(pd3dCommandList, 1);

	if (m_pMesh)
	{
		reinterpret_cast<CParticleMesh*>(m_pMesh)->PreRender(pd3dCommandList, 1); //Draw
		m_pMesh->Render(pd3dCommandList, 1); //Draw
	}
}

void CParticleObject::OnPostRender()
{
	if (m_pMesh)
	{
		reinterpret_cast<CParticleMesh*>(m_pMesh)->ParticlePostRender(0);
	}
}

void CParticleObject::SettingDetail(PARTICLE_SITUATION situation, int num)
{
	switch (situation)
	{
	case ID1_SKILL:
	case ID2_SKILL:
	case ID3_SKILL:
	case ID4_SKILL:
		switch (num)
		{
		case 24://Roar
			SetNum(100);
			SetSize(1.f);
			SetLife(1.2f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.7f, 0.16f, 0.08f, 1.f));
			break;
		case 25://Gluttony
			SetNum(100);
			SetSize(1.f);
			SetLife(1.2f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.01f, 0.65f, 0.08f, 1.f));
			break;
		case 26://Endure
			SetNum(100);
			SetSize(1.f);
			SetLife(1.2f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.7f, 0.16f, 0.08f, 1.f));
			break;
		case 28://DimensionPunch
			SetNum(1);
			SetSize(1.5f);
			SetLife(2.f);
			SetColor(XMFLOAT4(0.44f, 0.11f, 0.72f, 1.f));
			break;
		case 29://Butting
			SetNum(40);
			SetSize(0.5f);
			SetLife(3.f);
			SetColor(XMFLOAT4(0.44f, 0.11f, 0.72f, 1.f));
			break;
		case 34://--
			SetNum(1080);
			SetSize(0.7f);
			SetLife(1.5f);
			SetColor(XMFLOAT4(0.38f, 0.6f, 0.85f, 1));
			break;
		case 35://Release
			SetNum(60);
			SetSize(0.3f);
			SetLife(0.5f);
			SetColor(XMFLOAT4(0.44f, 0.11f, 0.72f, 1));
			break;
		case 36://Delete
			SetNum(1080);
			SetSize(0.7f);
			SetLife(1.5f);
			SetColor(XMFLOAT4(0.44f, 0.11f, 0.72f, 1.f));
			break;
		case 39://while true
			SetNum(1080);
			SetSize(0.7f);
			SetLife(1000.f);
			SetInfinity(true);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.9f, 0.88f, 0.4f, 1));
			break;
		case 48://back step
			SetNum(40);
			SetSize(0.3f);
			SetLife(0.25f);
			SetColor(XMFLOAT4(0.58f, 0.32f, 0.07f, 1.f));
			break; 
		case 49://Dodge
			SetNum(60);
			SetSize(0.55f);
			SetLife(0.8f);
			SetColor(XMFLOAT4(0.48f, 0.22f, 0.02f, 1.f));
			break;
		case 52://Vital Point
			SetNum(50);
			SetSize(0.55f);
			SetLife(0.3f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.47f, 0.15f, 0.10f, 1.f));
			break;
		case 55://Hunter Eyes
			SetNum(60);
			SetSize(0.75f);
			SetLife(0.4f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.87f, 0.15f, 0.10f, 1.f));
			break;
		case 56://wind step
			SetNum(40);
			SetSize(0.5f);
			SetLife(3.f);
			SetUpdatePosition(true);
			break;
		case 57://arrow rain
			SetNum(50);
			SetSize(1.f);
			SetLife(5.f);
			SetColor(XMFLOAT4(1.0f, 0.f, 0.f, 1.f));
			break;
		case 60://Fighter Dash
			SetNum(40);
			SetSize(0.3f);
			SetLife(0.25f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.58f, 0.32f, 0.07f, 1.f));
			break;
		case 61://Fighter Dodge
			SetNum(60);
			SetSize(0.55f);
			SetLife(0.8f);
			SetColor(XMFLOAT4(0.48f, 0.22f, 0.02f, 1.f));
			break;
		case 63://Wild Attack
			SetNum(20);
			SetSize(0.35f);
			SetLife(0.5f);
			SetColor(XMFLOAT4(0.94f, 0.5f, 0.2f, 1.f));
			break;
		case 64://명왕권
			SetNum(200);
			SetSize(0.5f);
			SetLife(3.f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.4f, 0.8f, 0.6f, 1.f));
			break;
		case 65://운기조식
			SetNum(400);
			SetSize(0.5f);
			SetLife(3.f);
			SetUpdatePosition(true);
			SetUpdateRotate(true);
			SetColor(XMFLOAT4(0.1f, 0.1f, 0.8f, 1.f));
			break;
		case 66://Point Blood
			SetNum(50);
			SetSize(0.55f);
			SetLife(0.3f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.47f, 0.15f, 0.10f, 1.f));
			break;
		case 67://Indestructible
			SetNum(50);
			SetSize(0.55f);
			SetLife(0.3f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.94f, 0.92f, 0.3f, 1.f));
			break;
		case 69://Counter
			SetNum(50);
			SetSize(0.3f);
			SetLife(0.5f);
			SetColor(XMFLOAT4(1.f, 0.67f, 0.1f, 1.f));
			break;
		case 70://FireBall
			SetNum(200);
			SetSize(0.5f);
			SetLife(1.5f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.65f, 0.1f, 0.f, 1.f));
			break;
		case 71://승룡각
			SetNum(30);
			SetSize(0.5f);
			SetLife(1.5f);
			SetColor(XMFLOAT4(0.7f, 0.5f, 0.1f, 1.f));
			break;
		case 72://SwordManDodge
			SetNum(60);
			SetSize(0.55f);
			SetLife(0.8f);
			SetColor(XMFLOAT4(0.48f, 0.22f, 0.02f, 1.f));
			break;
		case 73://RUN
			SetNum(50);
			SetSize(0.55f);
			SetLife(0.3f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.04f, 0.28f, 0.74f, 1.f));
			break;
		case 75://ShieldBash
			SetNum(100);
			SetSize(0.35f);
			SetLife(1.5f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.7f, 0.1f, 0.0f, 1.f));
			break;
		case 76://war_cry
			SetNum(30);
			SetSize(0.5f);
			SetLife(4.f);
			SetColor(XMFLOAT4(1.0f, 0.5f, 0.f, 1.f));
			break;
		case 77://Defensive Stance
			SetNum(30);
			SetSize(0.25f);
			SetLife(1.5f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.94f, 0.92f, 0.3f, 1.f));
			break;
		case 78://Berserk
			SetNum(60);
			SetSize(0.35f);
			SetLife(2.f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.7f, 0.1f, 0.0f, 1.f));
			break;
		case 80://Hell_blade
			/*SetNum(400);
			SetSize(0.5f);
			SetLife(2.5f);
			SetUpdatePosition(true);
			SetUpdateRotate(true);
			SetColor(XMFLOAT4(0.8f, 0.7, 0.4, 1));*/
			SetNum(1);
			SetSize(1.f);
			SetLife(3.f);
			SetUpdatePosition(true);
			SetUpdateRotate(true);
			SetboolLean(1);
			SetColor(XMFLOAT4(0.8f, 0.7f, 0.4f, 1.f));
			break;
		case 84://Teleport
			SetNum(50);
			SetSize(0.5f);
			SetLife(3.f);
			//SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.1f, 0.73f, 0.32f, 1.f));
			break;
		case 85://Blink
			SetNum(100);
			SetSize(0.4f);
			SetLife(0.5f);
			//SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.1f, 0.73f, 0.32f, 1.f));
			break;
		case 87://Enchant
			SetNum(300);
			SetSize(0.4f);
			SetLife(3.f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.9f, 0.3f, 0.1f, 1.f));
			break;
		case 88://EarthImpact
			SetNum(30);
			SetSize(0.7f);
			SetLife(3.f);
			SetColor(XMFLOAT4(0.2f, 0.5f, 0.4f, 1.f));
			break;
		case 90://EnergyBall
			SetNum(200);
			SetSize(0.5f);
			SetLife(1.5f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.96f, 0.96f, 0.28f, 1.f));
			break;
		case 91://Magic Eye
			SetNum(60);
			SetSize(0.75f);
			SetLife(0.4f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.28f, 0.42f, 0.96f, 1.f));
			break;
		case 93://Reflect
			SetNum(60);
			SetSize(0.55f);
			SetLife(0.8f);
			SetUpdatePosition(true);
			//SetColor(XMFLOAT4(0.32f, 0.33f, 0.4f, 1));
			SetColor(XMFLOAT4(0.96f, 0.96f, 0.28f, 1.f));
			break;
		case 95://OverLoad
			SetNum(60);
			SetSize(0.25f);
			SetLife(2.5f);
			SetUpdatePosition(true);
			SetColor(XMFLOAT4(0.2f, 0.5f, 0.4f, 1.f));
			break;
		default:
			break;
		}
		break;

	case JUMP:
		SetNum(1);
		SetSize(2.5f);
		SetLife(1.f);
		SetboolLean(1);
		SetColor(XMFLOAT4(0.2f, 0.5, 0.4, 1.f));
		break;
	case COINBYDEATH:
		SetNum(60);
		SetSize(0.25f);
		SetLife(1.5f);
		SetColor(XMFLOAT4(0.94f, 0.86f, 0.16f, 1.f));
		break;
	case FENCEEFFECT:
		SetNum(1400);
		SetSize(1.0f);
		SetLife(5.f);
		SetInfinity(true);
		SetColor(XMFLOAT4(0.46f, 0.36f, 1.f, 1));
		break;
	default:
		break;
	}
}

void CParticleObject::SettingDetail(SKILL_TYPE type, int num)
{
	switch (type)
	{
	case SKILL_TYPE::NONE:
		break;
	case SKILL_TYPE::ARCHER_ATTACK:
		SetNum(30);
		SetSize(0.2f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.1f, 0.4f, 0.8f, 1.f));
		break;
	case SKILL_TYPE::ARCHER_BACKSTEP:
		break;
	case SKILL_TYPE::ARCHER_DODGE:
		break;
	case SKILL_TYPE::ARCHER_MULTIPLE_SHOT:
		break;
	case SKILL_TYPE::ARCHER_VAULT:
		break;
	case SKILL_TYPE::ARCHER_PENETRAITING_SHOT:
		SetNum(30);
		SetSize(0.2f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.9f, 0.2f, 0.2f, 1));
		break;
	case SKILL_TYPE::ARCHER_STICKY_ARROW:
		SetNum(30);
		SetSize(0.2f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.f, 0.52f, 0.01f, 1));
		break;
	case SKILL_TYPE::ARCHER_PHOENIX_ARROW:
		SetNum(60);
		SetSize(0.5f);
		SetLife(3.f);
		SetColor(XMFLOAT4(1.f, 0.09f, 0.09f, 1));
		break;
	case SKILL_TYPE::ARCHER_STROM_ARROW:
		SetNum(100);
		SetSize(0.3f);
		SetLife(1.f);
		//SetColor(XMFLOAT4(1.f, 0.09f, 0.09f, 1));
		break;
	case SKILL_TYPE::ARCHER_ARROW_RAIN:
		SetNum(50);
		SetSize(1.f);
		SetLife(5.f);
		SetColor(XMFLOAT4(1.0f, 0, 0, 1));
		break;
	case SKILL_TYPE::FIGHTER_DODGE:
		break;
	case SKILL_TYPE::FIGTER_SPIN_KICK:
		break;
	case SKILL_TYPE::FIGHTER_WILD_ATTACK:
		break;
	case SKILL_TYPE::FIGHTER_WIND_KICK:
		break;
	case SKILL_TYPE::FIGHTER_FIREBALL:
		SetNum(200);
		SetSize(0.5f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.65f, 0.1f, 0.f, 1.f));
		break;
	case SKILL_TYPE::FIGHTER_RISING_DRAGON:
		break;
	case SKILL_TYPE::FIGHTER_MEDITATION:
		break;
	case SKILL_TYPE::FIGTHER_DRAGON_FIST:
		break;
	case SKILL_TYPE::FIGHTER_INDESTRUCTIBLE:
		break;
	case SKILL_TYPE::FIGHTER_COUNTER:
		break;
	case SKILL_TYPE::SWORDMAN_DODGE:
		break;
	case SKILL_TYPE::SWORDMAN_HEAVY_SLASH:
		break;
	case SKILL_TYPE::SWORDMAN_AURA_BLADE:
		SetNum(60);
		SetSize(0.3f);
		SetLife(3.f);
		break;
	case SKILL_TYPE::SWORDMAN_HELL_BLADE:
		break;
	case SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD:
		SetNum(1);
		SetSize(7.f);
		SetLife(3.f);
		SetColor(XMFLOAT4(1.0f, 0.2f, 0.2f, 1));
		break;
	case SKILL_TYPE::SWORDMAN_ANKLE_CUT:
		break;
	case SKILL_TYPE::SWORDMAN_SHIELD_BASH:
		break;
	case SKILL_TYPE::SWORDMAN_PROTECTED_AREA:
		break;
	case SKILL_TYPE::WIZARD_ATTACK:
		SetNum(100);
		SetSize(0.4f);
		SetLife(3.f);
		//SetColor(XMFLOAT4(0.3f, 0.7f, 1.f, 1.f));
		break;
	case SKILL_TYPE::WIZARD_TELEPORT:
		break;
	case SKILL_TYPE::WIZARD_EARTH_IMPACT:
		break;
	case SKILL_TYPE::WIZARD_MAGIC_MISSILE:
		SetNum(25);
		SetSize(0.4f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.5f, 0.f, 0.67f, 1.f));
		break;
	case SKILL_TYPE::WIZARD_MAGIC_EYE:
		break;
	case SKILL_TYPE::WIZARD_ENERGY_BALL:
		SetNum(200);
		SetSize(0.5f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.96f, 0.96f, 0.28f, 1.f));
		break;
	case SKILL_TYPE::WIZARD_DARKNESS_RAY:
		SetNum(10);
		SetSize(1.f);
		SetLife(2.f);
		SetUpdatePosition(true);
		SetColor(XMFLOAT4(0.32f, 0.33f, 0.4f, 1));
		break;
	case SKILL_TYPE::WIZARD_BIGBANG:
		SetNum(100);
		SetSize(0.4f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.3f, 0.3f, 0.3f, 1));
		//SetColor(XMFLOAT4(0.32f, 0.33f, 0.4f, 1));
		break;
	case SKILL_TYPE::WIZARD_BIGBANG_CONTINUE:
		SetNum(500);
		SetSize(1.0f);
		SetLife(3.f);
		SetUpdateRotate(true);
		SetColor(XMFLOAT4(0.3f, 0.3f, 0.3f, 1));
		break;
	case SKILL_TYPE::WIZARD_REFLECT:
		break;
	case SKILL_TYPE::WIZARD_OVERLOAD:
		break;
	case SKILL_TYPE::OGRE_ATTACK:
		break;
	case SKILL_TYPE::OGRE_HEAVY_SWING:
		break;
	case SKILL_TYPE::OGRE_CRUNCH:
		break;
	case SKILL_TYPE::OGRE_CHARGING:
		break;
	case SKILL_TYPE::OGRE_ROAR:
		break;
	case SKILL_TYPE::OGRE_ENDURE:
		break;
	case SKILL_TYPE::OGRE_GLUTTONY:
		break;
	case SKILL_TYPE::OGRE_ROCK_THROW:
		break;
	case SKILL_TYPE::OGRE_BUTTING:
		break;
	case SKILL_TYPE::OGRE_DIMENSION_PUNCH:
		break;
	case SKILL_TYPE::OGRE_DIMENSION_CRUSH:
		SetNum(2000);
		SetSize(5.0f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.43f, 0.0f, 0.72f, 1));
		break;
	case SKILL_TYPE::PRO_ATTACK:
		SetNum(100);
		SetSize(0.3f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.37f, 0.04f, 0.63f, 1));
		break;
	case SKILL_TYPE::PRO_POINTER:
		break;
	case SKILL_TYPE::PRO_RELEASE:
		break;
	case SKILL_TYPE::PRO_DELETE:
		break;
	case SKILL_TYPE::PRO_RETURN_ZERO:
		SetNum(100);
		SetSize(0.3f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.37f, 0.04f, 0.63f, 1));
		break;
	case SKILL_TYPE::PRO_SCL:
		SetNum(5);
		SetSize(1.f);
		SetLife(2.f);
		SetColor(XMFLOAT4(0.37f, 0.04f, 0.63f, 1));
		break;
	case SKILL_TYPE::PRO_WHILE_TRUE:
		break;
	case SKILL_TYPE::PRO_HELLO_WORLD:
		break;
	case SKILL_TYPE::BURN:
		break;
	case SKILL_TYPE::POISON:
		break;
	case SKILL_TYPE::MEMORY_LEAK:
		break;
	case SKILL_TYPE::SILENCE:
		break;
	case SKILL_TYPE::TOWERATTACK:
		SetNum(100);
		SetSize(0.3f);
		SetLife(3.f);
		SetColor(XMFLOAT4(0.07f, 0.16f, 0.63f, 1));
		break;
	default:
		break;
	}
}


void CParticleObject::AnimateSprite(float time)
{
	m_Time += time;
	if (m_Time > 0.1f)
	{
		m_Time = 0.f;
		m_iCurrentSpriteNum += 1;
		if (m_iTotalSpriteNum <= m_iCurrentSpriteNum)
		{
			m_iCurrentSpriteNum = 0;
		}
	}
}

bool CParticleObject::AnimateLifeTime(float time)
{
	if (!m_bInfinity)
	{
		if (m_bShow)
		{
			m_fTotalLifeTime -= time;
			if (m_fTotalLifeTime <= 0.f)
			{
				m_fTotalLifeTime = m_fLifeTime;
				SetShow(false);
				return false;
			}
			return true;
		}
	}
	return true;
}

CLobbyNpc::CLobbyNpc(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel)
{
	CLoadedModelInfo* pAngrybotModel = pModel;
	SetChild(pAngrybotModel->m_pModelRootObject, true);

	m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 1, pAngrybotModel);
	m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//Idle
	m_pSkinnedAnimationController->SetTrackEnable(0, true);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
}

CLobbyNpc::~CLobbyNpc()
{
}

CLobbyNpc* CLobbyNpc::CanTalk(XMFLOAT3 pos)
{
	XMFLOAT3 myPos = GetPosition();
	XMVECTOR vecDistance = XMVectorSubtract(XMLoadFloat3(&pos), XMLoadFloat3(&myPos));
	float squaredDistance = XMVectorGetX(XMVector3LengthSq(vecDistance));
	float squaredCognise = m_cognise * m_cognise;

	if (squaredDistance < squaredCognise)
		return this;
	else return nullptr;
}

void CLobbyNpc::Talk()
{
	if(onTalkCallback)
		onTalkCallback();
}
