//-----------------------------------------------------------------------------
// File: Shader.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Shader.h"
#include "Scene.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "Util.h"

uniform_int_distribution<> uid;
random_device rd;
default_random_engine dre(rd());

CShader::CShader()
{
}

CShader::~CShader()
{
	ReleaseShaderVariables();

	if (m_ppd3dPipelineStates)
	{
		bool release = true;

		for (int i = 0; i < m_nPipelineStates; i++) if (m_ppd3dPipelineStates[i])
			if (m_ppd3dPipelineStates[i]->Release() == 0)
				m_ppd3dPipelineStates[i] = nullptr;
			else
				release = false;

		if(release)
			delete[] m_ppd3dPipelineStates;
		//m_ppd3dPipelineStates = NULL;
	}
}

D3D12_SHADER_BYTECODE CShader::CreateVertexShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return(d3dShaderByteCode);
}

D3D12_SHADER_BYTECODE CShader::CreatePixelShader(int nPipelineState)
{
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;

	return(d3dShaderByteCode);
}

#define _WITH_WFOPEN
//#define _WITH_STD_STREAM

#ifdef _WITH_STD_STREAM
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#endif

D3D12_SHADER_BYTECODE CShader::ReadCompiledShaderFromFile(const WCHAR *pszFileName, ID3DBlob **ppd3dShaderBlob)
{
	UINT nReadBytes = 0;
#ifdef _WITH_WFOPEN
	FILE *pFile = NULL;
	::_wfopen_s(&pFile, pszFileName, L"rb");
	::fseek(pFile, 0, SEEK_END);
	int nFileSize = ::ftell(pFile);
	BYTE *pByteCode = new BYTE[nFileSize];
	::rewind(pFile);
	nReadBytes = (UINT)::fread(pByteCode, sizeof(BYTE), nFileSize, pFile);
	::fclose(pFile);
#endif
#ifdef _WITH_STD_STREAM
	std::ifstream ifsFile;
	ifsFile.open(pszFileName, std::ios::in | std::ios::ate | std::ios::binary);
	nReadBytes = (int)ifsFile.tellg();
	BYTE *pByteCode = new BYTE[*pnReadBytes];
	ifsFile.seekg(0);
	ifsFile.read((char *)pByteCode, nReadBytes);
	ifsFile.close();
#endif

	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	if (ppd3dShaderBlob)
	{
		*ppd3dShaderBlob = NULL;
		HRESULT hResult = D3DCreateBlob(nReadBytes, ppd3dShaderBlob);
		memcpy((*ppd3dShaderBlob)->GetBufferPointer(), pByteCode, nReadBytes);
		d3dShaderByteCode.BytecodeLength = (*ppd3dShaderBlob)->GetBufferSize();
		d3dShaderByteCode.pShaderBytecode = (*ppd3dShaderBlob)->GetBufferPointer();
	}
	else
	{
		d3dShaderByteCode.BytecodeLength = nReadBytes;
		d3dShaderByteCode.pShaderBytecode = pByteCode;
	}

	return(d3dShaderByteCode);
}

D3D12_INPUT_LAYOUT_DESC CShader::CreateInputLayout()
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = NULL;
	d3dInputLayoutDesc.NumElements = 0;

	return(d3dInputLayoutDesc);
}

D3D12_RASTERIZER_DESC CShader::CreateRasterizerState(int nPipelineState)
{
	if (nPipelineState == 1)
	{
		D3D12_RASTERIZER_DESC d3dRasterizerDesc;
		::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
		d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		d3dRasterizerDesc.FrontCounterClockwise = FALSE;
#ifdef _WITH_RASTERIZER_DEPTH_BIAS
		d3dRasterizerDesc.DepthBias = 250000;
#endif
		d3dRasterizerDesc.DepthBiasClamp = 0.0f;
		d3dRasterizerDesc.SlopeScaledDepthBias = 1.0f;
		d3dRasterizerDesc.DepthClipEnable = TRUE;
		d3dRasterizerDesc.MultisampleEnable = FALSE;
		d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
		d3dRasterizerDesc.ForcedSampleCount = 0;
		d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		return(d3dRasterizerDesc);
	}
	else
	{
		D3D12_RASTERIZER_DESC d3dRasterizerDesc;
		::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
		//	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
		d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
		d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
		d3dRasterizerDesc.FrontCounterClockwise = FALSE;
		d3dRasterizerDesc.DepthBias = 0;
		d3dRasterizerDesc.DepthBiasClamp = 0.0f;
		d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
		d3dRasterizerDesc.DepthClipEnable = TRUE;
		d3dRasterizerDesc.MultisampleEnable = FALSE;
		d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
		d3dRasterizerDesc.ForcedSampleCount = 0;
		d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		return(d3dRasterizerDesc);
	}
}

D3D12_DEPTH_STENCIL_DESC CShader::CreateDepthStencilState(int nPipelineState)
{
	if (nPipelineState == 1)
	{
		D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
		::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
		d3dDepthStencilDesc.DepthEnable = TRUE;
		d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; //D3D12_COMPARISON_FUNC_LESS_EQUAL
		d3dDepthStencilDesc.StencilEnable = FALSE;
		d3dDepthStencilDesc.StencilReadMask = 0x00;
		d3dDepthStencilDesc.StencilWriteMask = 0x00;
		d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

		return(d3dDepthStencilDesc);
	}
	else
	{ 
		D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
		::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
		d3dDepthStencilDesc.DepthEnable = TRUE;
		d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		d3dDepthStencilDesc.StencilEnable = FALSE;
		d3dDepthStencilDesc.StencilReadMask = 0x00;
		d3dDepthStencilDesc.StencilWriteMask = 0x00;
		d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
		d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
		d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

		return(d3dDepthStencilDesc);
	}
}

D3D12_BLEND_DESC CShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

void CShader::CreateShader(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = 1;
	m_d3dPipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	if (pd3dDevice == nullptr)
	{
		cout << "pd3dDevice nullptr" << endl;
	}

	if (m_ppd3dPipelineStates == nullptr)
	{
		cout << "ppd3dPipelineStates nullptr" << endl;
	}

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void **)&m_ppd3dPipelineStates[nPipelineState]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

void CShader::OnPrepareRender(ID3D12GraphicsCommandList *pd3dCommandList, int nPipelineState)
{
	if (m_ppd3dPipelineStates)
		pd3dCommandList->SetPipelineState(m_ppd3dPipelineStates[nPipelineState]);
}

void CShader::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int m_nPipelineStates)
{
	if(m_nPipelineStates != -1)OnPrepareRender(pd3dCommandList, m_nPipelineStates);
}

CTexture* CShader::LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* filePath)
{
	CTexture* texture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	texture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, filePath, RESOURCE_TEXTURE2D, 0);
	return texture;
}

CTexturedRectMesh* CShader::CreateTexturedRectMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float width, float height)
{
	return new CTexturedRectMesh(pd3dDevice, pd3dCommandList, width, height, 0.0f, 0.0f, 0.0f, 0.0f);
}

void CShader::CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dCbvSrvDescriptorHeap);

	m_d3dCbvCPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dCbvGPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_d3dSrvCPUDescriptorStartHandle.ptr = m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_d3dSrvGPUDescriptorStartHandle.ptr = m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);

	m_d3dSrvCPUDescriptorNextHandle = m_d3dSrvCPUDescriptorStartHandle;
	m_d3dSrvGPUDescriptorNextHandle = m_d3dSrvGPUDescriptorStartHandle;
}

void CShader::CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		D3D12_CPU_DESCRIPTOR_HANDLE d3dCbvCPUDescriptorHandle;
		d3dCbvCPUDescriptorHandle.ptr = m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * j);
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, d3dCbvCPUDescriptorHandle);
	}
}

void CShader::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	
	int nTextures = pTexture->GetTextures();
	UINT nTextureType = pTexture->GetTextureType();
	for (int i = 0; i < nTextures; i++)
	{
		ID3D12Resource* pShaderResource = pTexture->GetResource(i);
		if (pShaderResource)
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
			pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
			m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
			pTexture->SetGpuDescriptorHandle(i, m_d3dSrvGPUDescriptorNextHandle);
			m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		}
	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int i = 0; i < nRootParameters; i++) pTexture->SetRootParameterIndex(i, nRootParameterStartIndex + i);
}

void CShader::CreateShaderResourceViews(ID3D12Device* pd3dDevice, int nResources, ID3D12Resource** ppd3dResources, DXGI_FORMAT* pdxgiSrvFormats)
{
	for (int i = 0; i < nResources; i++)
	{
		if (ppd3dResources[i])
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
			d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			d3dShaderResourceViewDesc.Format = pdxgiSrvFormats[i];
			d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			d3dShaderResourceViewDesc.Texture2D.MipLevels = 1;
			d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
			d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
			d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
			pd3dDevice->CreateShaderResourceView(ppd3dResources[i], &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
			m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
			m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CSkyBoxShader::CSkyBoxShader()
{
}

CSkyBoxShader::~CSkyBoxShader()
{
}

D3D12_INPUT_LAYOUT_DESC CSkyBoxShader::CreateInputLayout()
{
	UINT nInputElementDescs = 1;
	D3D12_INPUT_ELEMENT_DESC *pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_DEPTH_STENCIL_DESC CSkyBoxShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0xff;
	d3dDepthStencilDesc.StencilWriteMask = 0xff;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_INCR;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_DECR;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CSkyBoxShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Skybox.hlsl", "VSSkyBox", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CSkyBoxShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Skybox.hlsl", "PSSkyBox", "ps_5_1", &m_pd3dPixelShaderBlob));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CStandardShader::CStandardShader()
{

}

CStandardShader::~CStandardShader()
{
}

void CStandardShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

D3D12_INPUT_LAYOUT_DESC CStandardShader::CreateInputLayout()
{
	UINT nInputElementDescs = 5;
	D3D12_INPUT_ELEMENT_DESC *pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

//D3D12_BLEND_DESC CStandardShader::CreateBlendState()
//{
//	D3D12_BLEND_DESC d3dBlendDesc;
//	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
//	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
//	d3dBlendDesc.IndependentBlendEnable = FALSE;
//	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
//	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
//	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
//	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//	return d3dBlendDesc;
//}

D3D12_SHADER_BYTECODE CStandardShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Standard.hlsl", "VSStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CStandardShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"DeferredRender.hlsl", "PSTexturedStandardMultipleRTs", "ps_5_1", &m_pd3dPixelShaderBlob));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CSkinnedAnimationStandardShader::CSkinnedAnimationStandardShader()
{
}

CSkinnedAnimationStandardShader::~CSkinnedAnimationStandardShader()
{
}

D3D12_INPUT_LAYOUT_DESC CSkinnedAnimationStandardShader::CreateInputLayout()
{
	UINT nInputElementDescs = 7;
	D3D12_INPUT_ELEMENT_DESC *pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[4] = { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 4, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[5] = { "BONEINDEX", 0, DXGI_FORMAT_R32G32B32A32_SINT, 5, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[6] = { "BONEWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 6, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_RASTERIZER_DESC CSkinnedAnimationStandardShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

//D3D12_BLEND_DESC CSkinnedAnimationStandardShader::CreateBlendState()
//{
//	D3D12_BLEND_DESC d3dBlendDesc;
//	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
//	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
//	d3dBlendDesc.IndependentBlendEnable = FALSE;
//	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
//	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
//	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
//	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
//	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
//	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
//	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
//	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
//	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
//	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
//
//	return d3dBlendDesc;
//}

D3D12_SHADER_BYTECODE CSkinnedAnimationStandardShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Animation.hlsl", "VSSkinnedAnimationStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CSkinnedAnimationStandardShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"DeferredRender.hlsl", "PSTexturedAnimationObjMultipleRTs", "ps_5_1", &m_pd3dPixelShaderBlob));
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CSkinnedAnimationObjectsShader::CSkinnedAnimationObjectsShader()
{
}

CSkinnedAnimationObjectsShader::~CSkinnedAnimationObjectsShader()
{
}

void CSkinnedAnimationObjectsShader::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CLoadedModelInfo *pModel, void *pContext)
{
}

void CSkinnedAnimationObjectsShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->Release();
		delete[] m_ppObjects;
	}
}

void CSkinnedAnimationObjectsShader::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;
}

void CSkinnedAnimationObjectsShader::ReleaseUploadBuffers()
{
	for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->ReleaseUploadBuffers();
}

void CSkinnedAnimationObjectsShader::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int nPipelineState)
{
	CSkinnedAnimationStandardShader::Render(pd3dCommandList, pCamera);

	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j])
		{
			m_ppObjects[j]->Animate(m_fElapsedTime);
			m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
	}
}

CPostProcessingShader::CPostProcessingShader()
{
}

CPostProcessingShader::~CPostProcessingShader()
{	
}

D3D12_INPUT_LAYOUT_DESC CPostProcessingShader::CreateInputLayout()
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = NULL;
	d3dInputLayoutDesc.NumElements = 0;

	return(d3dInputLayoutDesc);
}

D3D12_DEPTH_STENCIL_DESC CPostProcessingShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CPostProcessingShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"DeferredRender.hlsl", "VSScreenRectSamplingTextured", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CPostProcessingShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"DeferredRender.hlsl", "PSScreenRectSamplingTextured", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CPostProcessingShader::CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CPostProcessingShader::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
}

void CPostProcessingShader::ReleaseShaderVariables()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) m_ppObjects[j]->Release();
		delete[] m_ppObjects;
	}

	if (m_pTexture)
		delete m_pTexture;

	if (m_pd3dRtvCPUDescriptorHandles)
		delete[] m_pd3dRtvCPUDescriptorHandles;
}

void CPostProcessingShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{
	m_nObjects = 0;
}

void CPostProcessingShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera);
	pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	if (m_pTexture) m_pTexture->UpdateShaderVariables(pd3dCommandList);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}


void CPostProcessingShader::ForwardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j])
		{
			m_ppObjects[j]->Animate(m_fElapsedTime);
			m_ppObjects[j]->UpdateTransform(NULL);
			m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
	}
}


void CPostProcessingShader::CreateResourcesAndViews(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nResources, DXGI_FORMAT* pdxgiFormats, UINT nWidth, UINT nHeight, D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle, UINT nShaderResources)
{
	m_pTexture = new CTexture(nResources, RESOURCE_TEXTURE2D, 0, 1);

	D3D12_CLEAR_VALUE d3dClearValue = { DXGI_FORMAT_R8G8B8A8_UNORM, { 0.0f, 0.0f, 1.0f, 1.0f } };
	for (UINT i = 0; i < nResources; i++)
	{
		d3dClearValue.Format = pdxgiFormats[i];
		m_pTexture->CreateTexture(pd3dDevice, pd3dCommandList, i, RESOURCE_TEXTURE2D, nWidth, nHeight, 1, 0, pdxgiFormats[i], D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON, &d3dClearValue);
	}

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, nShaderResources);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateShaderResourceViews(pd3dDevice, m_pTexture, 0, 14);

	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;

	m_pd3dRtvCPUDescriptorHandles = new D3D12_CPU_DESCRIPTOR_HANDLE[nResources];

	for (UINT i = 0; i < nResources; i++)
	{
		d3dRenderTargetViewDesc.Format = pdxgiFormats[i];
		ID3D12Resource* pd3dTextureResource = m_pTexture->GetResource(i);
		pd3dDevice->CreateRenderTargetView(pd3dTextureResource, &d3dRenderTargetViewDesc, d3dRtvCPUDescriptorHandle);
		m_pd3dRtvCPUDescriptorHandles[i] = d3dRtvCPUDescriptorHandle;
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}

void CPostProcessingShader::OnPrepareRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList, int nRenderTargets, D3D12_CPU_DESCRIPTOR_HANDLE* pd3dRtvCPUHandles, D3D12_CPU_DESCRIPTOR_HANDLE d3dDepthStencilBufferDSVCPUHandle)
{
	int nResources = m_pTexture->GetTextures();
	D3D12_CPU_DESCRIPTOR_HANDLE* pd3dAllRtvCPUHandles = new D3D12_CPU_DESCRIPTOR_HANDLE[nRenderTargets + nResources];

	for (int i = 0; i < nRenderTargets; i++)
	{
		pd3dAllRtvCPUHandles[i] = pd3dRtvCPUHandles[i];
		pd3dCommandList->ClearRenderTargetView(pd3dRtvCPUHandles[i], Colors::Black, 0, NULL);
	}

	for (int i = 0; i < nResources; i++)
	{
		::SynchronizeResourceTransition(pd3dCommandList, GetTextureResource(i), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_RENDER_TARGET);

		D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = GetRtvCPUDescriptorHandle(i);
		FLOAT pfClearColor[4] = { 0.0f, 0.0f, 1.0f, 1.0f };
		pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor, 0, NULL);
		pd3dAllRtvCPUHandles[nRenderTargets + i] = d3dRtvCPUDescriptorHandle;
	}
	pd3dCommandList->OMSetRenderTargets(nRenderTargets + nResources, pd3dAllRtvCPUHandles, FALSE, &d3dDepthStencilBufferDSVCPUHandle);

	if (pd3dAllRtvCPUHandles) delete[] pd3dAllRtvCPUHandles;
}

void CPostProcessingShader::OnPostRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList)
{
	int nResources = m_pTexture->GetTextures();
	for (int i = 0; i < nResources; i++)
	{
		::SynchronizeResourceTransition(pd3dCommandList, GetTextureResource(i), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
	}
}



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CTextureShader* CTextureShader::s_instance = nullptr;

CTextureShader::CTextureShader()
{
}

CTextureShader::~CTextureShader()
{
}

CTextureShader* CTextureShader::Create()
{
	CTextureShader* pInstance = new CTextureShader();
	s_instance = pInstance;
	if (FAILED(pInstance->Initialize()))
	{
		SafeDelete(pInstance);
		return nullptr;
	}

	return pInstance;
}

HRESULT CTextureShader::Initialize()
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j) {
			m_iPlayerSkillArrays[i][j] = 0;
		}
		m_iClientSkillArray[i] = 0;
	}
	for (int i = 0; i < 3; ++i)
		m_ePlayerJOB[i] = JOB::ARCHER;

	return NOERROR;
}

void CTextureShader::Reset()
{
	m_setting = false;
	m_channel = false;
	m_randomShop = false;
	m_auction = false;
	m_blockchain = false;
	m_customize = false;
	m_bReadySceneSKillUI = false;
	m_shopping = false;
	m_accelerate = false;

	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j) {
			m_iPlayerSkillArrays[i][j] = 0;
		}
		m_iClientSkillArray[i] = 0;
	}
	for (int i = 0; i < 3; ++i)
		m_ePlayerJOB[i] = JOB::ARCHER;


}

void CTextureShader::DestroyInstance()
{
	s_instance->ReleaseObjects();
}

void CTextureShader::ResizeWindow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fResolution)
{
	if (m_nObjects) {
		for (int i = 0; i < m_nObjects; ++i) {
			if (m_ppObjects[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_ppObjects[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nUITextures) {
		for (int i = 0; i < m_nUITextures; ++i) {
			if (m_ppUITextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_ppUITextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nExtraTextures) {
		for (int i = 0; i < m_nExtraTextures; ++i) {
			if (m_ppExtraTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_ppExtraTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nShopTextures) {
		for (int i = 0; i < m_nShopTextures; ++i) {
			if (m_shopTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_shopTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nChannelTextures) {
		for (int i = 0; i < m_nChannelTextures; ++i) {
			if (m_channelTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_channelTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nAuctionTextures) {
		for (int i = 0; i < m_nAuctionTextures; ++i) {
			if (m_auctionTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_auctionTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nBlockchainTextures) {
		for (int i = 0; i < m_nBlockchainTextures; ++i) {
			if (m_blockchainTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_blockchainTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
	if (m_nCustomizeTextures) {
		for (int i = 0; i < m_nCustomizeTextures; ++i) {
			if (m_customizeTextures[i]) {
				dynamic_cast<CTexturedRectMesh*>(m_customizeTextures[i]->m_pMesh)->Resize(pd3dDevice, pd3dCommandList, fResolution);
			}
		}
	}
}

void CTextureShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_INPUT_LAYOUT_DESC CTextureShader::CreateInputLayout()
{
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

void CTextureShader::SetReadySceneSkillUI(bool b)
{
	const int nSkillWindows = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 12 : 10;
	m_bReadySceneSKillUI = b;
	
	for (int i = 0; i < nSkillWindows; ++i)
	{
		dynamic_cast<CTexturedRectMesh*>(m_ppObjects[i]->m_pMesh)->SetValue(b ? 1.0f : 0.0f);
		if(b)m_ppObjects[i]->DrawOn();
		else if(!b)m_ppObjects[i]->DrawOff();
	}
}

D3D12_RASTERIZER_DESC CTextureShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_DEPTH_STENCIL_DESC CTextureShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_BLEND_DESC CTextureShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

void CTextureShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{
	switch (m_CurScene)
	{
	case SCENEKIND::TITLE:
	{
		m_nObjects = 3;
		m_ppObjects = new CUIObject * [m_nObjects];

		CTexture* pTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/Title/WOD_Background_Title_Ver4.dds");
		CTexture* pSignInTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Button_SignIn.dds");
		CTexture* pSignUpTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Button_SignUp.dds");

		CScene::CreateShaderResourceViews(pd3dDevice, pTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, pSignInTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, pSignUpTexture, 0, 3);

		m_ppObjects[0] = new CUIObject(pd3dDevice, pd3dCommandList, pTexture, 
			CalculateScreenResolutionSize(1.0f, 1.0f), XMFLOAT2(231.0f, 0.0f), TEXTURETYPE::NONE, 0.0f, XMFLOAT2());
		m_ppObjects[1] = new CUIObject(pd3dDevice, pd3dCommandList, pSignInTexture,
			CalculateScreenResolutionSize(0.3203125f, 0.0810546875f), 
			CalculateScreenResolutionPos(0.1796875f, 0.74121f), TEXTURETYPE::BUTTON, 1.0f, XMFLOAT2());
		m_ppObjects[2] = new CUIObject(pd3dDevice, pd3dCommandList, pSignUpTexture,
			CalculateScreenResolutionSize(0.3203125f, 0.0810546875f),
			CalculateScreenResolutionPos(0.5f, 0.74121f), TEXTURETYPE::BUTTON, 1.0f, XMFLOAT2());

		// Set Sign In/Up Click Callback Functions
		function<void()> signInCallback =
			[obj = m_ppObjects[1]]()
		{
			char pass[NAME_SIZE];
			memset(SceneManager::GetInstance()->m_Name, 0, sizeof(SceneManager::GetInstance()->m_Name));
			WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, -1, SceneManager::GetInstance()->m_Name, NAME_SIZE, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.passBuf, -1, pass, NAME_SIZE, NULL, NULL);

#ifdef WITH_DATABASE
			NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name, pass);

#else
			if (wcscmp(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, L"") == 0) {
				char name[NAME_SIZE];
				memset(name, 0, sizeof(name));
				for (int i = 0; i < NAME_SIZE; ++i) {
					name[i] = 'A' + uid(dre) % 26;
				}
				name[NAME_SIZE - 1] = '\0';
				memcpy_s(SceneManager::GetInstance()->m_Name, NAME_SIZE, name, NAME_SIZE);
			}
			else {
				memset(SceneManager::GetInstance()->m_Name, 0, sizeof(SceneManager::GetInstance()->m_Name));
				WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, -1, SceneManager::GetInstance()->m_Name, NAME_SIZE, NULL, NULL);
			}
			NetworkManager::GetInstance()->playerScene = SCENEKIND::LOBBY;
#endif

			_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, NULL);
			_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.passBuf, NULL);
			memset(SceneManager::GetInstance()->m_TitleInfo.passShow, 0, sizeof(SceneManager::GetInstance()->m_TitleInfo.passShow));
			SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = false;
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppObjects[1]->SetOnClickCallback(signInCallback);

		function<void()> signUpCallback =
			[obj = m_ppObjects[2]]()
		{
#ifdef WITH_DATABASE
			char pass[NAME_SIZE];
			memset(SceneManager::GetInstance()->m_Name, 0, sizeof(SceneManager::GetInstance()->m_Name));
			WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, -1, SceneManager::GetInstance()->m_Name, NAME_SIZE, NULL, NULL);
			WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.passBuf, -1, pass, NAME_SIZE, NULL, NULL);
			NetworkManager::GetInstance()->SendSignUpPacket(SceneManager::GetInstance()->m_Name, pass);

			_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, NULL);
			_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.passBuf, NULL);
			memset(SceneManager::GetInstance()->m_TitleInfo.passShow, 0, sizeof(SceneManager::GetInstance()->m_TitleInfo.passShow));
			SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = false;
#endif
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppObjects[2]->SetOnClickCallback(signUpCallback);

		break;
	}
	case SCENEKIND::LOBBY:
	{
		m_auctionInfo = new LobbyAuctionInfo;
		m_auctionInfo->initialize();

		m_blockChainInfo = new LobbyBlockChainInfo;
		m_blockChainInfo->initialize();

		const int buttons = 3 + 3; // Matching Buttons (Hero, Boss, Matching) + Chat(type, input, list)
		m_nObjects = buttons; 
		m_ppObjects = new CUIObject * [buttons];

		CTexture* ppTextures[buttons];

		std::fstream in("Image/Lobby_Image.txt");
		if (in.fail())
			cout << "Failed to read file" << endl;

		int num = 0;
		string LobbyFileLoc;
		wstring wLobbyFileLoc;
		while (!in.eof())
		{
			in >> num;
			in >> LobbyFileLoc;
			wLobbyFileLoc.assign(LobbyFileLoc.begin(), LobbyFileLoc.end());
			ppTextures[num] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
			ppTextures[num]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, const_cast<wchar_t*>(wLobbyFileLoc.c_str()), RESOURCE_TEXTURE2D, 0);
		}
		for (int i = 0; i < m_nObjects; ++i)
			CScene::CreateShaderResourceViews(pd3dDevice, ppTextures[i], 0, 3);

		

		num = 0;

		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num], 
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.1f, FRAME_BUFFER_HEIGHT * 0.05f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.7740625f, FRAME_BUFFER_HEIGHT * 0.75f),
			TEXTURETYPE::BUTTON, 1.0f, XMFLOAT2() );
		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num],
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.1f, FRAME_BUFFER_HEIGHT * 0.05f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.85f, FRAME_BUFFER_HEIGHT * 0.75f),
			TEXTURETYPE::BUTTON, 1.0f, XMFLOAT2());
		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num],
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.2f, FRAME_BUFFER_HEIGHT * 0.1f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.7740625f, FRAME_BUFFER_HEIGHT * 0.8f),
			TEXTURETYPE::BUTTON, 1.0f, XMFLOAT2());
		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num],
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.07f, FRAME_BUFFER_HEIGHT * 0.04f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.02f, FRAME_BUFFER_HEIGHT * 0.91f),
			TEXTURETYPE::ALPHA, 0.7f, XMFLOAT2());
		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num],
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.3f, FRAME_BUFFER_HEIGHT * 0.04f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.08075f, FRAME_BUFFER_HEIGHT * 0.91f),
			TEXTURETYPE::ALPHA, 0.7f, XMFLOAT2());
		m_ppObjects[num++] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[num],
			XMFLOAT2(FRAME_BUFFER_RESIZE * 0.38f, FRAME_BUFFER_HEIGHT * 0.28f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.02f, FRAME_BUFFER_HEIGHT * 0.62f),
			TEXTURETYPE::ALPHA, 0.7f, XMFLOAT2());

		
		for (int i = 0; i < 2; ++i)
		{
			function<void()> onClickCallback = [obj = m_ppObjects[i]]() {
				bool clicked = obj->m_pMesh->GetClicked();
				obj->m_pMesh->SetValue(clicked == true ? 0.5f : 1.0f);
			};

			function<void()> onHoverCallback = [obj = m_ppObjects[i]]() {
				float val = obj->m_pMesh->GetValue();
				if(val == 1.0f)
					obj->m_pMesh->SetValue(1.5f);
			};

			function<void()> onHoverEndCallback = [obj = m_ppObjects[i]]() {
				float val = obj->m_pMesh->GetValue();
				if (val == 1.5f)
					obj->m_pMesh->SetValue(1.0f);
			};

			m_ppObjects[i]->SetOnClickCallback(onClickCallback);
			m_ppObjects[i]->SetOnReleaseCallback(nullptr);
			m_ppObjects[i]->SetOnHoverCallback(onHoverCallback);
			m_ppObjects[i]->SetOnHoverEndCallback(onHoverEndCallback);
		}
		
		// For Lobby Buttons + interaction + cash
		m_nUITextures = 5;
		m_ppUITextures = new CUIObject * [m_nUITextures];

		CTexture* textures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/Setting.dds"), // Setting
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/Channel.dds"), // Channel
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/Party.dds"), // Party
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/Cash.dds"), // Cash 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/PressE.dds"), // PressE
		};
		XMFLOAT2 sizes[] = {
			CalculateScreenResolutionSize(0.0602f, 0.08125f),
			CalculateScreenResolutionSize(0.0602f, 0.08125f),
			CalculateScreenResolutionSize(0.0602f, 0.08125f),
			CalculateScreenResolutionSize(0.1963f, 0.0625f),
			CalculateScreenResolutionSize(0.2787f, 0.0625f)
		};
		XMFLOAT2 positions[] = {
			CalculateScreenResolutionSize(0.0f, 0.0f),
			CalculateScreenResolutionSize(0.0602f, 0.0f),
			CalculateScreenResolutionSize(0.1204f, 0.0f),
			XMFLOAT2(FRAME_BUFFER_WIDTH * 0.8037f, 0.0f),
			CalculateScreenResolutionPos(0.3815f, 0.6325f)
		};

		for (int i = 0; i < m_nUITextures; ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, textures[i], 0, 3);
			TEXTURETYPE type = i < 3 ? TEXTURETYPE::BUTTON : TEXTURETYPE::NONE;
			m_ppUITextures[i] = new CUIObject(pd3dDevice, pd3dCommandList, textures[i], sizes[i], positions[i], type,
				1.0f, XMFLOAT2());
		}
		function<void()> settingButtonCallback = [this, obj = m_ppUITextures[0]]() {
			this->SettingSwitch();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppUITextures[0]->SetOnClickCallback(settingButtonCallback);

		function<void()> channelCallback = [obj = m_ppUITextures[1]]() {
			CTextureShader::GetInstance()->ChannelSwitch();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppUITextures[1]->SetOnClickCallback(channelCallback);

		m_nExtraTextures = NumSetting + NumCheckBox + NumCheckIcon + NumPlusButton + NumMinusButton + NumFullScreen + NumWindowMode + NumAccept + NumReset; 
		m_ppExtraTextures = new CUIObject * [m_nExtraTextures];
		int numSettings[] = { NumSetting, NumCheckBox, NumCheckIcon, NumPlusButton, NumMinusButton, NumFullScreen, NumWindowMode, NumAccept, NumReset };

		CTexture* settingTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/SettingWindow.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/SlotBox.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/Icon_Check.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Plus.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Minus.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/Full_Screen.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/Window_Screen.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/Accept.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Setting/Reset.dds"), 
		};

		XMFLOAT2 settingSize = XMFLOAT2();
		XMFLOAT2 settingPos = XMFLOAT2();
		TEXTURETYPE settigType = TEXTURETYPE::NONE;
		function<void()> settingFunc;
		int currentNum = 0;
		for (int i = 0; i < sizeof(numSettings) / sizeof(numSettings[0]); ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, settingTextures[i], 0, 3);
			for (int j = 0; j < numSettings[i]; ++j)
			{
				switch (i)
				{
				case 0:
				{
					settingPos = CalculateScreenResolutionPos(0.0f, 0.0f);
					settingSize = CalculateScreenResolutionSize(1.0f, 1.0f);
					break;
				}
				case 1:
				{
					settingSize = CalculateScreenResolutionSize(0.032407f, 0.04375f);
					settingPos = j < 5 ? CalculateScreenResolutionPos(0.612963f, 0.1475f + (0.04375f * j)) :	//hdr
						j == 5 ? CalculateScreenResolutionPos(0.53148f, 0.3825f) :	// outline on
						j == 6 ? CalculateScreenResolutionPos(0.612963f, 0.3825f) :	// off
						j == 7 ? CalculateScreenResolutionPos(0.53148f, 0.75375f) :	// shadow on
						CalculateScreenResolutionPos(0.612963f, 0.75375f);				// off
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 2:
				{
					settingSize = CalculateScreenResolutionSize(0.032407f, 0.04375f);
					settingPos = j == 0 ? CalculateScreenResolutionPos(0.612963f, 0.1475f + (0.04375f * 3)) :
						j == 1 ? CalculateScreenResolutionPos(0.53148f, 0.3825f) :
						CalculateScreenResolutionPos(0.53148f, 0.75375f);
					break;
				}
				case 3:
				{
					settingSize = j < 4 ? CalculateScreenResolutionSize(0.022222f, 0.03f) :
						CalculateScreenResolutionSize(0.032407f, 0.04375f);
					settingPos = j < 4 ? CalculateScreenResolutionPos(0.64537f, 0.4425f + (0.03625f * j)) :
						CalculateScreenResolutionPos(0.56852f, 0.5975f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 4:
				{
					settingSize = j < 4 ? CalculateScreenResolutionSize(0.022222f, 0.03f) :
						CalculateScreenResolutionSize(0.032407f, 0.04375f);
					settingPos = j < 4 ? CalculateScreenResolutionPos(0.61759f, 0.4425f + (0.03625f * j)) :
						CalculateScreenResolutionPos(0.4287f, 0.5975f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 5:
				{
					settingSize = CalculateScreenResolutionSize(0.17315f, 0.04f);
					settingPos = CalculateScreenResolutionPos(0.47222f, 0.65625f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 6:
				{
					settingSize = CalculateScreenResolutionSize(0.17315f, 0.04f);
					settingPos = CalculateScreenResolutionPos(0.47222f, 0.69625f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 7:
				{
					settingSize = CalculateScreenResolutionSize(0.12315f, 0.06f);
					settingPos = CalculateScreenResolutionPos(0.37593f, 0.815f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				case 8:
				{
					settingSize = CalculateScreenResolutionSize(0.12315f, 0.06f);
					settingPos = CalculateScreenResolutionPos(0.50185f, 0.815f);
					settigType = TEXTURETYPE::BUTTON;
					break;
				}
				}

				m_ppExtraTextures[currentNum] = new CUIObject(pd3dDevice, pd3dCommandList, settingTextures[i], settingSize, settingPos, 
					settigType, 1.0f, XMFLOAT2());
				m_ppExtraTextures[currentNum]->DrawOff();
				currentNum++;
			}
		}

		for (int i = 0; i < NumCheckBox; ++i)
		{
			int index = NumSetting + i;
			if (i < 5)
			{
				settingFunc = [option = static_cast<UINT>(i), obj = m_ppExtraTextures[index], check = m_ppExtraTextures[10]]() {
					SceneManager::GetInstance()->m_nDrawOption = option; 
					check->SetScreenPosition(obj->GetScreenPosition()); 
					obj->m_pMesh->SetValue(0.5f);
				};
			}
			else if (i == 5) {
				settingFunc = [obj = m_ppExtraTextures[index], check = m_ppExtraTextures[11]]() {
					SceneManager::GetInstance()->m_outline = true;
					check->SetScreenPosition(obj->GetScreenPosition());
					obj->m_pMesh->SetValue(0.5f);
				};
			}
			else if (i == 6) {
				settingFunc = [obj = m_ppExtraTextures[index], check = m_ppExtraTextures[11]]() {
					SceneManager::GetInstance()->m_outline = false;
					check->SetScreenPosition(obj->GetScreenPosition());
					obj->m_pMesh->SetValue(0.5f);
				};
			}
			else if (i == 7) {
				settingFunc = [obj = m_ppExtraTextures[index], check = m_ppExtraTextures[12]]() {
					SceneManager::GetInstance()->m_shadow = true;
					check->SetScreenPosition(obj->GetScreenPosition());
					obj->m_pMesh->SetValue(0.5f);
				};
			}
			else {
				settingFunc = [obj = m_ppExtraTextures[index], check = m_ppExtraTextures[12]]() {
					SceneManager::GetInstance()->m_shadow = false;
					check->SetScreenPosition(obj->GetScreenPosition());
					obj->m_pMesh->SetValue(0.5f);
				};
			}
			
			if (settingFunc)m_ppExtraTextures[index]->SetOnClickCallback(settingFunc);
		}

		for (int i = 0; i < NumPlusButton; ++i) {
			int index = NumSetting + NumCheckBox + NumCheckIcon + i;
			switch (i)
			{
			case 0:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fExposure += SceneManager::GetInstance()->m_fExposure < 2.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 1:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fSaturation += SceneManager::GetInstance()->m_fSaturation < 2.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 2:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fContrast += SceneManager::GetInstance()->m_fContrast < 2.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 3:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fVibrance += SceneManager::GetInstance()->m_fVibrance < 2.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 4:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					CTextureShader::GetInstance()->AddVolume();
					int vol = CTextureShader::GetInstance()->GetVolume();
					SoundManager::GetInstance()->ChangeVolume(VOLUME_INT_TO_FLOAT(vol));
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			}
			if (settingFunc)m_ppExtraTextures[index]->SetOnClickCallback(settingFunc);
		}

		for (int i = 0; i < NumMinusButton; ++i) {
			int index = NumSetting + NumCheckBox + NumCheckIcon + NumPlusButton + i;
			switch (i)
			{
			case 0:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fExposure -= SceneManager::GetInstance()->m_fExposure > 0.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 1:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fSaturation -= SceneManager::GetInstance()->m_fSaturation > 0.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 2:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fContrast -= SceneManager::GetInstance()->m_fContrast > 0.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 3:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					SceneManager::GetInstance()->m_fVibrance -= SceneManager::GetInstance()->m_fVibrance > 0.0f ? 0.1f : 0.0f;
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			case 4:
			{
				settingFunc = [obj = m_ppExtraTextures[index]]() {
					CTextureShader::GetInstance()->SubtractVolume();
					int vol = CTextureShader::GetInstance()->GetVolume();
					SoundManager::GetInstance()->ChangeVolume(VOLUME_INT_TO_FLOAT(vol));
					obj->m_pMesh->SetValue(0.5f);
				};
				break;
			}
			}
			if (settingFunc)m_ppExtraTextures[index]->SetOnClickCallback(settingFunc);
		}

		function<void()> fullScreenCallback = [obj = m_ppExtraTextures[23]]() {
			//implement fullscreen
			obj->m_pMesh->SetValue(0.5f);
		};

		m_ppExtraTextures[23]->SetOnClickCallback(fullScreenCallback);

		function<void()> windowScreenCallback = [obj = m_ppExtraTextures[24]]() {
			//implement windowmode
			obj->m_pMesh->SetValue(0.5f); 
		}; 

		m_ppExtraTextures[24]->SetOnClickCallback(windowScreenCallback);

		function<void()> acceptCallBack = [obj = m_ppExtraTextures[25]]() {   
			CTextureShader::GetInstance()->SettingSwitch();  
			obj->m_pMesh->SetValue(0.5f);  
		};  
		m_ppExtraTextures[25]->SetOnClickCallback(acceptCallBack);

		function<void()> resetCallback = [obj = m_ppExtraTextures[26]]() {
			CTextureShader::GetInstance()->SettingReset();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppExtraTextures[26]->SetOnClickCallback(resetCallback);

		/////////////////////////////////
		// For Channel

		m_nChannelTextures = 22;
		CTexture* channelTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Channel/ChannelBg.dds");
		CTexture* channelButtonTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Channel/ChannelButtons.dds");

		CScene::CreateShaderResourceViews(pd3dDevice, channelTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, channelButtonTexture, 0, 3);

		for (int i = 0; i < m_nChannelTextures; ++i)
		{
			CTexture* texture = i < 21 ? channelButtonTexture : channelTexture;
			float xOffset = static_cast<float>(i % 7);
			float yOffset = static_cast<float>(i / 7);
			XMFLOAT2 pos = i < 21 ? CalculateScreenResolutionPos(0.0963f + (xOffset * 0.11574f), 0.53625f + (yOffset * 0.07875f)) :
				CalculateScreenResolutionPos(0.0f, 0.0f);
			XMFLOAT2 size = i < 21 ? CalculateScreenResolutionSize(0.11574f, 0.07875f) :
				CalculateScreenResolutionSize(1.0f, 1.0f);
			TEXTURETYPE type = i < 21 ? TEXTURETYPE::CHANNELBUTTON : TEXTURETYPE::NONE;
			float u = xOffset * (1.0f/7.0f);
			float v = yOffset * (1.0f/3.0f);
			XMFLOAT2 uvOffset = i < 21 ? XMFLOAT2(u, v) : XMFLOAT2();
			m_channelTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, texture, size, pos, type, 1.0f, uvOffset));
			m_channelTextures[i]->DrawOff();
			if (i < 21) {
				m_channelTextures[i]->SetBasicButtonEvents();
				function<void()> channelCallback;
				if (i != 20) {
					channelCallback = [obj = m_channelTextures[i], index = i]() {
						NetworkManager::GetInstance()->SendChangeChannelPacket(index);
						obj->m_pMesh->SetValue(1.0f);
					};
				}
				else {
					channelCallback = [obj = m_channelTextures[i]]() {
						CTextureShader::GetInstance()->ChannelSwitch();
						obj->m_pMesh->SetValue(1.0f);
					};
				}
				m_channelTextures[i]->SetOnClickCallback(channelCallback);
			}
		}

		m_nShopTextures = 6;
		CTexture* pShopTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/RandomShopBg.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/Purchase.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Left.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Right.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/ItemSet.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/QuestionIcon.dds"),
		};

		XMFLOAT2 shopPositions[] = {
			CalculateScreenResolutionPos(0.0f, 0.0f),
			CalculateScreenResolutionPos(0.28796f, 0.35875f),
			CalculateScreenResolutionPos(0.35648f, 0.245f),
			CalculateScreenResolutionPos(0.57315f, 0.245f),
			CalculateScreenResolutionPos(0.43148f, 0.4925f),
			CalculateScreenResolutionPos(0.43148f, 0.4925f)
		};

		XMFLOAT2 shopSizes[] = {
			CalculateScreenResolutionSize(1.0f, 1.0f),
			CalculateScreenResolutionSize(0.425f, 0.13375f),
			CalculateScreenResolutionSize(0.0713f, 0.09625f),
			CalculateScreenResolutionSize(0.0713f, 0.09625f),
			CalculateScreenResolutionSize(0.14074f, 0.19f),
			CalculateScreenResolutionSize(0.14074f, 0.19f)
		};

		TEXTURETYPE shopTypes[] = { 
			TEXTURETYPE::TWINKLE,
			TEXTURETYPE::BUTTON,
			TEXTURETYPE::BUTTON,
			TEXTURETYPE::BUTTON,
			TEXTURETYPE::CUSTOMPARTS,
			TEXTURETYPE::NONE

		};

		for (int i = 0; i < m_nShopTextures; ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, pShopTextures[i], 0, 3);
			float xOffset = 0;
			float yOffset = 13;
			float u = xOffset * (1.0f / 38.0f);
			float v = yOffset * (1.0f / 40.0f);
			XMFLOAT2 uvOffset = i == 4 ? XMFLOAT2(u, v) : XMFLOAT2();
			m_shopTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, pShopTextures[i], shopSizes[i], shopPositions[i],
				shopTypes[i], 1.0f, uvOffset));
			m_shopTextures[i]->DrawOff();
		}
		
		function<void()> leftCallback = [obj = m_shopTextures[2]]() {
			CTextureShader::GetInstance()->SubtractCurParts();
			obj->m_pMesh->SetValue(0.5f);
		};
		function<void()> rightCallback = [obj = m_shopTextures[3]]() {
			CTextureShader::GetInstance()->AddCurParts();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_shopTextures[2]->SetOnClickCallback(leftCallback);
		m_shopTextures[3]->SetOnClickCallback(rightCallback);

		function<void()> randomCallback = [this, obj = m_shopTextures[1]]() {
			if (!m_startRandom) {
				CTextureShader::GetInstance()->RandomStart();
				NetworkManager::GetInstance()->SendShopPacket();
				obj->m_pMesh->SetValue(0.5f);
				SoundManager::GetInstance()->Play_Sound(L"LobbyShopRandom.wav", CHANNELID::EFFECT, 0.8f);
			}
		};
		m_shopTextures[1]->SetOnClickCallback(randomCallback);

		m_nAuctionTextures = NumAuction + NumLeftIcon + NumRightIcon + NumCalc + NumMyPart + NumProduct + NumRegist + NumQuit;

		int numAuctionTexture[] = { NumAuction, NumLeftIcon, NumRightIcon, NumCalc, NumMyPart, NumProduct, NumRegist, NumQuit };
		CTexture* auctionTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Auction/AuctionBg.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Left.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Right.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Auction/NumKeyboard.dds"),
			nullptr,
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Auction/Upload.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Auction/Quit.dds")
		};
		
		for (int i = 0; i < 7; ++i) {
			if(i != 4)CScene::CreateShaderResourceViews(pd3dDevice, auctionTextures[i], 0, 3);
		}

		int curNum = 0;
		for (int i = 0; i < sizeof(numAuctionTexture) / sizeof(numAuctionTexture[0]); ++i) 
		{
			for (int j = 0; j < numAuctionTexture[i]; ++j) 
			{
				XMFLOAT2 size = XMFLOAT2(); 
				XMFLOAT2 pos = XMFLOAT2();
				TEXTURETYPE type = TEXTURETYPE::NONE;
				float val = 1.0f; 
				XMFLOAT2 uvOffset = XMFLOAT2();
				int textureIndex = i < 5 ? i : i - 1;
				switch (i)
				{
				case 0:
				{
					size = CalculateScreenResolutionSize(1.0f, 1.0f); 
					pos = CalculateScreenResolutionPos(0.0f, 0.0f);
					break;
				}
				case 1:
				{
					size = j < 2 ? CalculateScreenResolutionSize(0.0426f, 0.0575f) 
						: CalculateScreenResolutionSize(0.066667f, 0.09f); 
					pos = j < 2 ? CalculateScreenResolutionPos(0.51852f, 0.2575f + (j * 0.0875f))
						: CalculateScreenResolutionPos(0.1713f, 0.79625f);
					type = TEXTURETYPE::BUTTON; 
					break;
				}
				case 2:
				{
					size = j < 2 ? CalculateScreenResolutionSize(0.0426f, 0.0575f)
						: CalculateScreenResolutionSize(0.066667f, 0.09f);
					pos = j < 2 ? CalculateScreenResolutionPos(0.65f, 0.2575f + (j * 0.0875f))
						: CalculateScreenResolutionPos(0.37222f, 0.79625f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 3:
				{
					int xOffset = j % 3;
					int yOffset = j / 3;
					size = CalculateScreenResolutionSize(0.05f, 0.0675f);
					pos = CalculateScreenResolutionPos(0.62222f + (0.05f * xOffset), 0.50375f + (0.0675f * yOffset));
					type = TEXTURETYPE::CALCKEYBOARD;
					float u = xOffset * (1.0f / 3.0f);
					float v = yOffset * (1.0f / 4.0f);
					uvOffset = XMFLOAT2(u, v);
					break;
				}
				case 4:
				{
					size = CalculateScreenResolutionSize(0.1287f, 0.17375f);
					pos = CalculateScreenResolutionPos(0.72222f, 0.2275f);
					type = TEXTURETYPE::CUSTOMPARTS;
					break;
				}
				case 5:
				{
					size = CalculateScreenResolutionSize(0.050926f, 0.06875f);
					pos = CalculateScreenResolutionPos(0.11204f, 0.28875f + 0.06875f * j);
					type = TEXTURETYPE::CUSTOMPARTS;
					break;
				}
				case 6:
				{
					size = CalculateScreenResolutionSize(0.125926f, 0.085f);
					pos = CalculateScreenResolutionPos(0.5704f, 0.7875f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 7:
				{
					size = CalculateScreenResolutionSize(0.125926f, 0.085f);
					pos = CalculateScreenResolutionPos(0.696326f, 0.7875f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				}
				CTexture* texture = textureIndex == 4 ? pShopTextures[4] : auctionTextures[textureIndex];
				m_auctionTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, texture, size, pos, type,
					val, uvOffset)); 
				m_auctionTextures[curNum]->DrawOff();
				curNum++;
			}
		}

		for (int i = 0; i < NumProduct; ++i) {
			int index = NumAuction + NumLeftIcon + NumRightIcon + NumCalc + NumMyPart + i;
			m_auctionTextures[index]->SetBasicButtonEvents();
		}
		for (int i = 0; i < NumCalc; ++i) {
			int index = NumAuction + NumLeftIcon + NumRightIcon + i;
			m_auctionTextures[index]->SetBasicButtonEvents();
		}

		function<void()> subPartsCallback = [obj = m_auctionTextures[1], parts = m_auctionTextures[19]]() {
			CTextureShader::GetInstance()->m_auctionInfo->curParts--;
			CTextureShader::GetInstance()->m_auctionInfo->curIndex = 0;
			int curParts = CTextureShader::GetInstance()->m_auctionInfo->curParts;
			if (curParts < 0)
				CTextureShader::GetInstance()->m_auctionInfo->curParts = 26;

			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(curParts, 0);
			dynamic_cast<CTexturedRectMesh*>(parts->m_pMesh)->SetUV(uvOffset);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[1]->SetOnClickCallback(subPartsCallback);

		function<void()> subNumCallback = [obj = m_auctionTextures[2], parts = m_auctionTextures[19]]() {
			short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;

			const vector<int> maxParts = {
				11, 4, 13, 38, 13, 15, 21, 21, 6, 6, 12, 11, 11, 3,
				22, 13, sex == 1 ? 7 : 10, 28, 20, 20, 18, 18,17, 17, 28, 19, 19
			};
			int curParts = CTextureShader::GetInstance()->m_auctionInfo->curParts;
			CTextureShader::GetInstance()->m_auctionInfo->curIndex--;
			int curIndex = CTextureShader::GetInstance()->m_auctionInfo->curIndex;
			if (curIndex < 0)
				CTextureShader::GetInstance()->m_auctionInfo->curIndex = maxParts[curParts] - 1;
			curIndex = CTextureShader::GetInstance()->m_auctionInfo->curIndex;

			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(curParts, curIndex);
			dynamic_cast<CTexturedRectMesh*>(parts->m_pMesh)->SetUV(uvOffset);

			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[2]->SetOnClickCallback(subNumCallback);

		function<void()> subPageCallback = [obj = m_auctionTextures[3]]() {
			CTextureShader::GetInstance()->m_auctionInfo->curPage--;
			if (CTextureShader::GetInstance()->m_auctionInfo->curPage < 0)
				CTextureShader::GetInstance()->m_auctionInfo->curPage = CTextureShader::GetInstance()->m_auctionInfo->maxPageNum - 1;

			/*implement Page Refresh*/
			NetworkManager::GetInstance()->SendGetAuctionInfoPacket(CTextureShader::GetInstance()->m_auctionInfo->curPage);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[3]->SetOnClickCallback(subPageCallback);

		function<void()> addPartsCallback = [obj = m_auctionTextures[4], parts = m_auctionTextures[19]]() {
			CTextureShader::GetInstance()->m_auctionInfo->curParts++;
			CTextureShader::GetInstance()->m_auctionInfo->curIndex = 0;
			int curParts = CTextureShader::GetInstance()->m_auctionInfo->curParts;
			if (curParts > 26)
				CTextureShader::GetInstance()->m_auctionInfo->curParts = 0;
			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(curParts, 0);
			dynamic_cast<CTexturedRectMesh*>(parts->m_pMesh)->SetUV(uvOffset);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[4]->SetOnClickCallback(addPartsCallback);

		function<void()> addNumCallback = [obj = m_auctionTextures[5], parts = m_auctionTextures[19]]() {
			short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;

			const vector<int> maxParts = {
				11, 4, 13, 38, 13, 15, 21, 21, 6, 6, 12, 11, 11, 3,
				22, 13, sex == 1 ? 7 : 10, 28, 20, 20, 18, 18,17, 17, 28, 19, 19
			};
			int curParts = CTextureShader::GetInstance()->m_auctionInfo->curParts;
			CTextureShader::GetInstance()->m_auctionInfo->curIndex++;
			int curIndex = CTextureShader::GetInstance()->m_auctionInfo->curIndex;
			if (curIndex >= maxParts[curParts])
				CTextureShader::GetInstance()->m_auctionInfo->curIndex = 0;
			curIndex = CTextureShader::GetInstance()->m_auctionInfo->curIndex;

			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(curParts, curIndex);
			dynamic_cast<CTexturedRectMesh*>(parts->m_pMesh)->SetUV(uvOffset);

			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[5]->SetOnClickCallback(addNumCallback);

		function<void()> addPageCallback = [obj = m_auctionTextures[6]]() {
			CTextureShader::GetInstance()->m_auctionInfo->curPage++;
			int maxPage = CTextureShader::GetInstance()->m_auctionInfo->maxPageNum;
			if (CTextureShader::GetInstance()->m_auctionInfo->curPage >= maxPage)
				CTextureShader::GetInstance()->m_auctionInfo->curPage = 0;

			/*implement Page Refresh*/
			NetworkManager::GetInstance()->SendGetAuctionInfoPacket(CTextureShader::GetInstance()->m_auctionInfo->curPage);

			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[6]->SetOnClickCallback(addPageCallback);

		for (int i = 7; i <= 15; ++i) {
			function<void()> oneCallback = [obj = m_auctionTextures[i], index = i]() {
				int price = CTextureShader::GetInstance()->m_auctionInfo->sellPrice;
				CTextureShader::GetInstance()->m_auctionInfo->sellPrice = price * 10 + (index - 7 + 1);
				obj->m_pMesh->SetValue(0.5f);
			};
			m_auctionTextures[i]->SetOnClickCallback(oneCallback);
		}

		function<void()> tenCallback = [obj = m_auctionTextures[16]]() {
			int price = CTextureShader::GetInstance()->m_auctionInfo->sellPrice;
			CTextureShader::GetInstance()->m_auctionInfo->sellPrice = price * 10;
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[16]->SetOnClickCallback(tenCallback);

		function<void()> hundredCallback = [obj = m_auctionTextures[17]]() {
			int price = CTextureShader::GetInstance()->m_auctionInfo->sellPrice;
			CTextureShader::GetInstance()->m_auctionInfo->sellPrice = price * 100;
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[17]->SetOnClickCallback(hundredCallback);

		function<void()> subCallback = [obj = m_auctionTextures[18]]() {
			int price = CTextureShader::GetInstance()->m_auctionInfo->sellPrice;
			CTextureShader::GetInstance()->m_auctionInfo->sellPrice = price / 10;
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[18]->SetOnClickCallback(subCallback);

		for (int i = 20; i <= 26; ++i) {
			function<void()> buyCallback = [this, obj = m_auctionTextures[i], index = i]() {
				/*implement here buy Customize Parts*/
				NetworkManager::GetInstance()->SendBuyAuctionPacket(m_auctionInfo->username[index - 20], m_auctionInfo->productParts[index - 20], m_auctionInfo->productNum[index - 20], m_auctionInfo->productPrice[index - 20]);
				obj->m_pMesh->SetValue(0.5f);
				NetworkManager::GetInstance()->SendOpenAuctionPacket();
				AuctionSwitch();
				AuctionSwitch();
			};
			m_auctionTextures[i]->SetOnClickCallback(buyCallback);
		}

		function<void()> registCallback = [obj = m_auctionTextures[27]]() {
			/*implement here regist my Parts*/
			NetworkManager::GetInstance()->SendRegisterAuctionPacket(SceneManager::GetInstance()->m_Name, static_cast<SHOP_TYPE>(CTextureShader::GetInstance()->m_auctionInfo->curParts), CTextureShader::GetInstance()->m_auctionInfo->curIndex, CTextureShader::GetInstance()->m_auctionInfo->sellPrice);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[27]->SetOnClickCallback(registCallback);

		function<void()> quitCallback = [obj = m_auctionTextures[28]]() {
			CTextureShader::GetInstance()->AuctionSwitch();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_auctionTextures[28]->SetOnClickCallback(quitCallback);

		m_nBlockchainTextures = NumBlockChain + NumInputBar + NumCalc + NumStaking + NumQuit;
		int numBlockChainTexture[] = { NumBlockChain, NumInputBar, NumCalc, NumStaking, NumQuit };

		CTexture* blockchainTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/BlockChain/BlockChainBg.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/BlockChain/TypingBar.dds"), 
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Auction/NumKeyboard.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/BlockChain/Staking.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/BlockChain/QuitBlockChain.dds"),
		};

		curNum = 0;
		for (int i = 0; i < sizeof(numBlockChainTexture) / sizeof(numBlockChainTexture[0]); ++i) {
			CScene::CreateShaderResourceViews(pd3dDevice, blockchainTextures[i], 0, 3);
			for (int j = 0; j < numBlockChainTexture[i]; ++j) {
				XMFLOAT2 size = XMFLOAT2();
				XMFLOAT2 pos = XMFLOAT2();
				TEXTURETYPE type = TEXTURETYPE::NONE;
				float val = 1.0f;
				XMFLOAT2 uvOffset = XMFLOAT2();
				switch (i)
				{
				case 0:
				{
					size = CalculateScreenResolutionSize(1.0f, 1.0f);
					pos = CalculateScreenResolutionPos(0.0f, 0.0f);
					break;
				}
				case 1:
				{
					size = CalculateScreenResolutionSize(0.205556f, 0.06875f);
					pos = CalculateScreenResolutionPos(0.5074f, 0.22f + (j * 0.14375f));
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 2:
				{
					int xOffset = j % 3;
					int yOffset = j / 3;
					size = CalculateScreenResolutionSize(0.0601852f, 0.08125f);
					pos = CalculateScreenResolutionPos(0.51852f+ (0.0601852f * xOffset), 0.46875f + (0.08125f * yOffset));
					type = TEXTURETYPE::CALCKEYBOARD;
					float u = xOffset * (1.0f / 3.0f);
					float v = yOffset * (1.0f / 4.0f);
					uvOffset = XMFLOAT2(u, v);
					break;
				}
				case 3:
				{
					size = CalculateScreenResolutionSize(0.1574f, 0.06375f);
					pos = CalculateScreenResolutionPos(0.530556f, 0.81625f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 4:
				{
					size = CalculateScreenResolutionSize(0.1574f, 0.06375f);
					pos = CalculateScreenResolutionPos(0.312963f, 0.81625f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				}

				m_blockchainTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, blockchainTextures[i], size, pos, type,
					val, uvOffset));
				m_blockchainTextures[curNum]->DrawOff();
				curNum++;
			}
		}

		for (int i = 0; i < NumCalc; ++i) {
			int index = NumBlockChain + NumInputBar + i;
			m_blockchainTextures[index]->SetBasicButtonEvents();
		}

		function<void()> inputTokenCallback = [obj = m_blockchainTextures[1]]() {
			CTextureShader::GetInstance()->SetinputKind(false);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[1]->SetOnClickCallback(inputTokenCallback);

		function<void()> inputPeriodCallback = [obj = m_blockchainTextures[2]]() {
			CTextureShader::GetInstance()->SetinputKind(true);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[2]->SetOnClickCallback(inputPeriodCallback);

		for (int i = 3; i <= 11; ++i) {
			function<void()> oneCallback = [obj = m_blockchainTextures[i], index = i]() {
				bool isPeriod = CTextureShader::GetInstance()->GetinputKind();
				cout << isPeriod << endl;
				auto info = CTextureShader::GetInstance()->m_blockChainInfo;

				int input = isPeriod ? info->stakingPeriod : info->inputTokens;
				if (isPeriod) {
					CTextureShader::GetInstance()->m_blockChainInfo->stakingPeriod = input * 10 + (index - 3 + 1);
				}
				else {
					CTextureShader::GetInstance()->m_blockChainInfo->inputTokens = input * 10 + (index - 3 + 1);
				}
				obj->m_pMesh->SetValue(0.5f);
			};
			m_blockchainTextures[i]->SetOnClickCallback(oneCallback);
		}

		function<void()> mulTenCallback = [obj = m_blockchainTextures[12]]() {
			bool isPeriod = CTextureShader::GetInstance()->GetinputKind();

			auto info = CTextureShader::GetInstance()->m_blockChainInfo;

			int input = isPeriod ? info->stakingPeriod : info->inputTokens;
			if (isPeriod) {
				CTextureShader::GetInstance()->m_blockChainInfo->stakingPeriod = input * 10;
			}
			else {
				CTextureShader::GetInstance()->m_blockChainInfo->inputTokens = input * 10;
			}
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[12]->SetOnClickCallback(mulTenCallback);

		function<void()> mulHundredCallback = [obj = m_blockchainTextures[13]]() {
			bool isPeriod = CTextureShader::GetInstance()->GetinputKind();

			auto info = CTextureShader::GetInstance()->m_blockChainInfo;

			int input = isPeriod ? info->stakingPeriod : info->inputTokens;
			if (isPeriod) {
				CTextureShader::GetInstance()->m_blockChainInfo->stakingPeriod = input * 100;
			}
			else {
				CTextureShader::GetInstance()->m_blockChainInfo->inputTokens = input * 100;
			}
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[13]->SetOnClickCallback(mulHundredCallback);

		function<void()> deleteCallback = [obj = m_blockchainTextures[14]]() {
			bool isPeriod = CTextureShader::GetInstance()->GetinputKind();

			auto info = CTextureShader::GetInstance()->m_blockChainInfo;

			int input = isPeriod ? info->stakingPeriod : info->inputTokens;
			if (isPeriod) {
				CTextureShader::GetInstance()->m_blockChainInfo->stakingPeriod = input / 10;
			}
			else {
				CTextureShader::GetInstance()->m_blockChainInfo->inputTokens = input / 10;
			}
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[14]->SetOnClickCallback(deleteCallback);		

		function<void()> stakingCallback = [this, obj = m_blockchainTextures[15]]() {
			/*implement here for staking tokens	*/
			NetworkManager::GetInstance()->SendStakeTokenPacket(m_blockChainInfo->inputTokens, m_blockChainInfo->stakingPeriod);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[15]->SetOnClickCallback(stakingCallback);

		function<void()> quitBlockChainCallback = [obj = m_blockchainTextures[16]]() {
			CTextureShader::GetInstance()->BlockChainSwitch();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_blockchainTextures[16]->SetOnClickCallback(quitBlockChainCallback);

		m_nCustomizeTextures = NumCustomize + NumLeftArrow + NumRightArrow + NumItemParts + NumApply + NumQuit + NumLiftUp;
		int numCustomizeTexture[] = { NumCustomize, NumLeftArrow, NumRightArrow, NumItemParts, NumApply, NumQuit, NumLiftUp };

		CTexture* customizeTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Customize/Customize.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Left.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Right.dds"),
			nullptr,
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Customize/Apply.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/BlockChain/QuitBlockChain.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Customize/liftUp.dds")
		};

		curNum = 0;
		for (int i = 0; i < sizeof(numCustomizeTexture) / sizeof(numCustomizeTexture[0]); ++i) {
			if(i!=3) CScene::CreateShaderResourceViews(pd3dDevice, customizeTextures[i], 0, 3);
			for (int j = 0; j < numCustomizeTexture[i]; ++j) {
				XMFLOAT2 size = XMFLOAT2();
				XMFLOAT2 pos = XMFLOAT2();
				TEXTURETYPE type = TEXTURETYPE::NONE;
				float val = 1.0f;
				XMFLOAT2 uvOffset = XMFLOAT2();
				CTexture* texture = i == 3 ? pShopTextures[4] : customizeTextures[i];
				switch (i)
				{
				case 0:
				{
					size = CalculateScreenResolutionSize(0.924f, 0.935f);
					pos = CalculateScreenResolutionPos(0.038f, 0.0325f);
					break;
				}
				case 1:
				{
					size = CalculateScreenResolutionSize(0.086111f, 0.11625f);
					pos = CalculateScreenResolutionPos(0.5543f, 0.13f + (j * 0.1825f));
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 2:
				{
					size = CalculateScreenResolutionSize(0.086111f, 0.11625f);
					pos = CalculateScreenResolutionPos(0.81852f, 0.13f + (j * 0.1825f));
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 3:
				{
					size = CalculateScreenResolutionSize(0.2287f, 0.30875f);
					pos = CalculateScreenResolutionPos(0.6176f, 0.42875f);
					uvOffset = GetCustomizeUvOffset(14, 0);
					type = TEXTURETYPE::CUSTOMPARTS;
					break;
				}
				case 4:
				{
					size = CalculateScreenResolutionSize(0.27315f, 0.08625f);
					pos = CalculateScreenResolutionPos(0.594445f, 0.755f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 5:
				{
					size = CalculateScreenResolutionSize(0.15926f, 0.06125f);
					pos = CalculateScreenResolutionPos(0.6565f, 0.8525f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				case 6:
				{
					size = XMFLOAT2(401.0f, 94.0f);
					pos = XMFLOAT2(419.0f, 947.0f);
					type = TEXTURETYPE::BUTTON;
					break;
				}
				}

				m_customizeTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, texture, size, pos, type,
					val, uvOffset));
				m_customizeTextures[curNum]->DrawOff();
				curNum++;
			}
		}

		function<void()> customizePartsMinusCallback = [obj = m_customizeTextures[1], texture = m_customizeTextures[5]]() {
			CTextureShader::GetInstance()->m_curParts--; 
			if (CTextureShader::GetInstance()->m_curParts < 0)
				CTextureShader::GetInstance()->m_curParts = 26;
			auto info = NetworkManager::GetInstance()->lobbySceneInfo; 
			while (!info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size()) {
				CTextureShader::GetInstance()->m_curParts--;
				if (CTextureShader::GetInstance()->m_curParts < 0)
					CTextureShader::GetInstance()->m_curParts = 26;
			}
			CTextureShader::GetInstance()->m_indexCustomize = 0;
			while (!info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].second) {
				CTextureShader::GetInstance()->m_indexCustomize++;
				if (CTextureShader::GetInstance()->m_indexCustomize > info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size() - 1) {
					CTextureShader::GetInstance()->m_indexCustomize = 0;
				}
			}
			CTextureShader::GetInstance()->m_nCurParts = info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].first;
			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(CTextureShader::GetInstance()->m_curParts, CTextureShader::GetInstance()->m_nCurParts);

			dynamic_cast<CTexturedRectMesh*>(texture->m_pMesh)->SetUV(uvOffset);

			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[1]->SetOnClickCallback(customizePartsMinusCallback);

		function<void()> customizeNumMinusCallback = [obj = m_customizeTextures[2], texture = m_customizeTextures[5]]() {
			auto info = NetworkManager::GetInstance()->lobbySceneInfo;

			CTextureShader::GetInstance()->m_indexCustomize--;
			if (CTextureShader::GetInstance()->m_indexCustomize < 0) {
				CTextureShader::GetInstance()->m_indexCustomize = info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size() - 1;
			}
			
			CTextureShader::GetInstance()->m_nCurParts = info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].first;
			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(CTextureShader::GetInstance()->m_curParts, CTextureShader::GetInstance()->m_nCurParts);
			dynamic_cast<CTexturedRectMesh*>(texture->m_pMesh)->SetUV(uvOffset);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[2]->SetOnClickCallback(customizeNumMinusCallback);

		function<void()> customizePartsPlusCallback = [obj = m_customizeTextures[3], texture = m_customizeTextures[5]]() {
			CTextureShader::GetInstance()->m_curParts++;
			if (CTextureShader::GetInstance()->m_curParts > 26)
				CTextureShader::GetInstance()->m_curParts = 0;
			auto info = NetworkManager::GetInstance()->lobbySceneInfo;
			while (!info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size()) {
				CTextureShader::GetInstance()->m_curParts++;
				if (CTextureShader::GetInstance()->m_curParts > 26)
					CTextureShader::GetInstance()->m_curParts = 0;
			}
			CTextureShader::GetInstance()->m_indexCustomize = 0;
			while (!info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].second) {
				CTextureShader::GetInstance()->m_indexCustomize++;
				if (CTextureShader::GetInstance()->m_indexCustomize > info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size() - 1) {
					CTextureShader::GetInstance()->m_indexCustomize = 0;
				}
			}
			CTextureShader::GetInstance()->m_nCurParts = info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].first;
			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(CTextureShader::GetInstance()->m_curParts, CTextureShader::GetInstance()->m_nCurParts);

			dynamic_cast<CTexturedRectMesh*>(texture->m_pMesh)->SetUV(uvOffset); 

			obj->m_pMesh->SetValue(0.5f); 
		};
		m_customizeTextures[3]->SetOnClickCallback(customizePartsPlusCallback);

		function<void()> customizeNumPlusCallback = [obj = m_customizeTextures[4], texture = m_customizeTextures[5]]() {
			auto info = NetworkManager::GetInstance()->lobbySceneInfo;

			CTextureShader::GetInstance()->m_indexCustomize++;
			if (CTextureShader::GetInstance()->m_indexCustomize > info->customizeDatas[CTextureShader::GetInstance()->m_curParts].size() - 1) {
				CTextureShader::GetInstance()->m_indexCustomize = 0;
			}

			CTextureShader::GetInstance()->m_nCurParts = info->customizeDatas[CTextureShader::GetInstance()->m_curParts][CTextureShader::GetInstance()->m_indexCustomize].first;
			XMFLOAT2 uvOffset = CTextureShader::GetInstance()->GetCustomizeUvOffset(CTextureShader::GetInstance()->m_curParts, CTextureShader::GetInstance()->m_nCurParts);
			dynamic_cast<CTexturedRectMesh*>(texture->m_pMesh)->SetUV(uvOffset);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[4]->SetOnClickCallback(customizeNumPlusCallback);

		function<void()> customizeApplyCallback = [obj = m_customizeTextures[6]]() {
			ModelCustomize model = reinterpret_cast<CPlayerObject*>(NetworkManager::GetInstance()->myClient)->GetCustomizeInfo();
			
			switch (CTextureShader::GetInstance()->m_curParts) {
			case 0:
				model.Chr_HeadCoverings_Base_Hair = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 1:
				model.Chr_HeadCoverings_No_FacialHair = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 2:
				model.Chr_HeadCoverings_No_Hair = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 3:
				model.Chr_Hair = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 4:
				model.Chr_HelmetAttachment = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 5:
				model.Chr_BackAttachment = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 6:
				model.Chr_ShoulderAttachRight = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 7:
				model.Chr_ShoulderAttachLeft = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 8:
				model.Chr_ElbowAttachRight = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 9:
				model.Chr_ElbowAttachLeft = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 10:
				model.Chr_HipsAttachment = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 11:
				model.Chr_KneeAttachRight = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 12:
				model.Chr_KneeAttachLeft = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 13:
				model.Chr_Ear_Ear = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 14:
				model.Chr_Head = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 15:
				model.Chr_Head_No_Elements = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 16:
				model.Chr_Eyebrow = CTextureShader::GetInstance()->m_nCurParts + 1;
				break;
			case 17:
				model.Chr_Torso = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 18:
				model.Chr_ArmUpperRight = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 19:
				model.Chr_ArmUpperLeft = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 20:
				model.Chr_ArmLowerRight = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 21:
				model.Chr_ArmLowerLeft = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 22:
				model.Chr_HandRight = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 23:
				model.Chr_HandLeft = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 24:
				model.Chr_Hips = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 25:
				model.Chr_LegRight = CTextureShader::GetInstance()->m_nCurParts;
				break;
			case 26:
				model.Chr_LegLeft = CTextureShader::GetInstance()->m_nCurParts;
				break;
			}
			reinterpret_cast<CPlayerObject*>(NetworkManager::GetInstance()->myClient)->Customize(model);
			NetworkManager::GetInstance()->SendModelCustomizePacket(model);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[6]->SetOnClickCallback(customizeApplyCallback);

		function<void()> customizeQuitCallback = [obj = m_customizeTextures[7]]() {
			CTextureShader::GetInstance()->CustomizeSwitch();
			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[7]->SetOnClickCallback(customizeQuitCallback);

		function<void()> customizeLiftUpCallback = [obj = m_customizeTextures[8]]() {
			ModelCustomize model = reinterpret_cast<CPlayerObject*>(NetworkManager::GetInstance()->myClient)->GetCustomizeInfo();

			switch (CTextureShader::GetInstance()->m_curParts) {
			case 0:
				model.Chr_HeadCoverings_Base_Hair = -1;
				break;
			case 1:
				model.Chr_HeadCoverings_No_FacialHair = -1;
				break;
			case 2:
				model.Chr_HeadCoverings_No_Hair = -1;
				break;
			case 3:
				model.Chr_Hair = -1;
				break;
			case 4:
				model.Chr_HelmetAttachment = -1;
				break;
			case 5:
				model.Chr_BackAttachment = -1;
				break;
			case 6:
				model.Chr_ShoulderAttachRight = -1;
				break;
			case 7:
				model.Chr_ShoulderAttachLeft = -1;
				break;
			case 8:
				model.Chr_ElbowAttachRight = -1;
				break;
			case 9:
				model.Chr_ElbowAttachLeft = -1;
				break;
			case 10:
				model.Chr_HipsAttachment = -1;
				break;
			case 11:
				model.Chr_KneeAttachRight = -1;
				break;
			case 12:
				model.Chr_KneeAttachLeft = -1;
				break;
			case 13:
				model.Chr_Ear_Ear = -1;
				break;
			case 14:
				model.Chr_Head = -1;
				break;
			case 15:
				model.Chr_Head_No_Elements = -1;
				break;
			case 16:
				model.Chr_Eyebrow = -1;
				break;
			case 17:
				model.Chr_Torso = -1;
				break;
			case 18:
				model.Chr_ArmUpperRight = -1;
				break;
			case 19:
				model.Chr_ArmUpperLeft = -1;
				break;
			case 20:
				model.Chr_ArmLowerRight = -1;
				break;
			case 21:
				model.Chr_ArmLowerLeft = -1;
				break;
			case 22:
				model.Chr_HandRight = -1;
				break;
			case 23:
				model.Chr_HandLeft = -1;
				break;
			case 24:
				model.Chr_Hips = -1;
				break;
			case 25:
				model.Chr_LegRight = -1;
				break;
			case 26:
				model.Chr_LegLeft = -1;
				break;
			}
			reinterpret_cast<CPlayerObject*>(NetworkManager::GetInstance()->myClient)->Customize(model);
			NetworkManager::GetInstance()->SendModelCustomizePacket(model);
			obj->m_pMesh->SetValue(0.5f);
		};
		m_customizeTextures[8]->SetOnClickCallback(customizeLiftUpCallback);

		break; 
	}
	case SCENEKIND::READY:
	{
		// Skill Icons Build
		const int nJobs = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 3 : 1; 
		const int nSkillWindows = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 12 : 10; 
		const int nSelectedSkills = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 4 * 3 : 4; 
		const int nSkills = nSkillWindows + nSelectedSkills;
		const int nIcons = nJobs + nSkills;

		m_nObjects = nIcons;
		m_ppObjects = new CUIObject * [nIcons];

		CTexture* pSkillIconTexture = LoadTexture(pd3dDevice, pd3dCommandList, SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? L"Image/Player_SKillSet.dds" : L"Image/Boss_SKillSet.dds"); 
		CTexture* pJobIconTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/JobIconSet.dds"); 
		CScene::CreateShaderResourceViews(pd3dDevice, pSkillIconTexture, 0, 3); 
		CScene::CreateShaderResourceViews(pd3dDevice, pJobIconTexture, 0, 3); 

		for (int i = 0; i < m_nObjects; ++i)
		{
			XMFLOAT2 rectSize = i < nSkillWindows ? CalculateScreenResolutionSize(0.06f, 0.08f) :
				CalculateScreenResolutionSize(0.035f, 0.045f);
			TEXTURETYPE type = i < nSkills ? (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? TEXTURETYPE::PLAYERSKILL : TEXTURETYPE::BOSSSKILL) : TEXTURETYPE::JOBKIND;
			CTexture* pTexture = i < nSkills ? pSkillIconTexture : pJobIconTexture;
			float texVal = 1.0f;
			XMFLOAT2 uvOffset = XMFLOAT2();
			XMFLOAT2 pos = XMFLOAT2();

			if (i >= nSkills) // only job textures
			{
				float u = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ?
					static_cast<float>(static_cast<int>(m_ePlayerJOB[i - nSkills]) / 6.0f) :
					static_cast<float>(static_cast<int>(m_eBossJOB) + 4) / 6.0f;
				uvOffset = XMFLOAT2(u, 0.0f);
				texVal = 1.0f;
			}

			if (i < nSkillWindows)
			{
				int column = i % 4;
				int row = i / 4;
				float strideX = FRAME_BUFFER_RESIZE * 0.06f;
				float strideY = FRAME_BUFFER_HEIGHT * 0.08f;
				pos = CalculateScreenResolutionPos(0.31f, 0.6f);
				pos.x += strideX * column;
				pos.y += strideY * row;
			}
			else if (i < nSkills)
			{
				int index = i - nSkillWindows;
				int column = index % 4;
				int row = index / 4;
				float strideX = 0.3f;
				float strideY = FRAME_BUFFER_HEIGHT * 0.05f;

				if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) {
					if (row == 2) {
						pos = CalculateScreenResolutionPos(0.302f + strideX * row, 0.503f);
					}
					else {
						pos = CalculateScreenResolutionPos(0.322f + strideX * row, 0.503f);
					}
					pos.y -= column * strideY;
				}
				else {
					pos = CalculateScreenResolutionPos(0.32f + index * 0.04f, 0.465f);
				}
			}
			else
			{
				int row = i - nSkills;
				float offset = row == 2 ? 0.29f : 0.3f;
				pos = CalculateScreenResolutionPos(0.32f + row * offset, 0.28f);
			}
			m_ppObjects[i] = new CUIObject(pd3dDevice, pd3dCommandList, pTexture, rectSize, pos, type, 1.0f, uvOffset);
		}

		// UI Textures Build
		if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
		{
			//////////
			const int uiIcons = 2 + 2 + 2 + 12 + 1; // Chat, Select, Skill/Stat, Slot, Ready
			m_nUITextures = uiIcons;
			m_ppUITextures = new CUIObject * [m_nUITextures];

			fstream ui("Image/Ready_Player_UI.txt");
			if (ui.fail())
				cout << "Failed to read file" << endl;

			vector<CTexture*> ppTextures(m_nUITextures);
			int num = 0;
			string UIFileLoc;
			wstring wUIFileLoc;
			while (!ui.eof())
			{
				ui >> num;
				ui >> UIFileLoc;
				wUIFileLoc.assign(UIFileLoc.begin(), UIFileLoc.end());
				ppTextures[num] = LoadTexture(pd3dDevice, pd3dCommandList, const_cast<wchar_t*>(wUIFileLoc.c_str()));
			}

			vector<XMFLOAT2> sizes = {
				CalculateScreenResolutionSize(0.23f, 0.28f), // Chat Icon
				CalculateScreenResolutionSize(0.23f, 0.04f),
				CalculateScreenResolutionSize(0.04f, 0.05f), // Stat/Skill Button
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.23f, 0.05f), // Stat/Skill Image
				CalculateScreenResolutionSize(0.23f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f), // Slot
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.13f, 0.06f) // Ready
			};

			std::vector<XMFLOAT2> positions = {
				CalculateScreenResolutionSize(0.02f, 0.59f), // Chat Icon
				CalculateScreenResolutionSize(0.02f, 0.88f),
				CalculateScreenResolutionPos(0.53f, 0.87f), // Stat/Skill Button				
				CalculateScreenResolutionPos(0.88f, 0.87f),
				CalculateScreenResolutionPos(0.3f, 0.87f),  // Stat/Skill Image
				CalculateScreenResolutionPos(0.65f, 0.87f),
				CalculateScreenResolutionPos(0.32f, 0.5f), // Slot
				CalculateScreenResolutionPos(0.32f, 0.45f),
				CalculateScreenResolutionPos(0.32f, 0.4f),
				CalculateScreenResolutionPos(0.32f, 0.35f),
				CalculateScreenResolutionPos(0.62f, 0.5f),
				CalculateScreenResolutionPos(0.62f, 0.45f),
				CalculateScreenResolutionPos(0.62f, 0.4f),
				CalculateScreenResolutionPos(0.62f, 0.35f),
				CalculateScreenResolutionPos(0.9f, 0.5f),
				CalculateScreenResolutionPos(0.9f, 0.45f),
				CalculateScreenResolutionPos(0.9f, 0.4f),
				CalculateScreenResolutionPos(0.9f, 0.35f),
				CalculateScreenResolutionSize(0.02f, 0.075f) // Ready
			};
			std::vector<TEXTURETYPE> types = {
				TEXTURETYPE::ALPHA, TEXTURETYPE::ALPHA, // Chat Icon
				TEXTURETYPE::BUTTON, TEXTURETYPE::BUTTON, // Stat/Skill Button
				TEXTURETYPE::NONE, TEXTURETYPE::NONE, // Stat/Skill Image
				TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE, // Slot
				TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,
				TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,
				TEXTURETYPE::BUTTON // Ready
			};

			for (int i = 0; i < m_nUITextures; ++i)
			{
				CScene::CreateShaderResourceViews(pd3dDevice, ppTextures[i], 0, 3);
				float val = types[i] == TEXTURETYPE::ALPHA ? 0.7f : 1.0f;
				m_ppUITextures[i] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[i],
					sizes[i], positions[i], types[i], val, XMFLOAT2());
			}
		}
		else
		{
			///////////
			const int uiIcons = 2 + 2 + 4 + 1;// Select, Skill/Stat, Slot, Ready
			m_nUITextures = uiIcons;
			m_ppUITextures = new CUIObject * [m_nUITextures];

			std::fstream ui("Image/Ready_Boss_UI.txt");
			if (ui.fail())
				cout << "Failed to read file" << endl;

			vector<CTexture*> ppTextures(m_nObjects);

			int num = 0;
			string UIFileLoc;
			wstring wUIFileLoc;
			while (!ui.eof())
			{
				ui >> num;
				ui >> UIFileLoc;
				wUIFileLoc.assign(UIFileLoc.begin(), UIFileLoc.end());
				ppTextures[num] = LoadTexture(pd3dDevice, pd3dCommandList, const_cast<wchar_t*>(wUIFileLoc.c_str()));
			}

			vector<XMFLOAT2> sizes = {
				CalculateScreenResolutionSize(0.04f, 0.05f), // Stat/Skill Button
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.23f, 0.05f), // Stat/Skill Image
				CalculateScreenResolutionSize(0.23f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f), // Slot
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.04f, 0.05f),
				CalculateScreenResolutionSize(0.13f, 0.06f) // Ready
			};

			vector<XMFLOAT2> positions = {
				CalculateScreenResolutionPos(0.53f, 0.87f), // Stat/Skill Button
				CalculateScreenResolutionPos(0.88f, 0.87f),
				CalculateScreenResolutionPos(0.3f, 0.87f), // Stat/Skill Image
				CalculateScreenResolutionPos(0.65f, 0.87f),
				CalculateScreenResolutionPos(0.32f, 0.46f), // Slot
				CalculateScreenResolutionPos(0.36f, 0.46f),
				CalculateScreenResolutionPos(0.40f, 0.46f),
				CalculateScreenResolutionPos(0.44f, 0.46f),
				CalculateScreenResolutionSize(0.02f, 0.075f) // Ready
			};

			std::vector<TEXTURETYPE> types = {
				TEXTURETYPE::BUTTON, TEXTURETYPE::BUTTON, // Stat/Skill Button 
				TEXTURETYPE::NONE, TEXTURETYPE::NONE, // Stat/Skill Image 
				TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE,TEXTURETYPE::NONE, // Slot
				TEXTURETYPE::BUTTON // Ready 
			};

			for (int i = 0; i < m_nUITextures; ++i)
			{
				CScene::CreateShaderResourceViews(pd3dDevice, ppTextures[i], 0, 3);
				m_ppUITextures[i] = new CUIObject(pd3dDevice, pd3dCommandList, ppTextures[i],
					sizes[i], positions[i], types[i], 1.0f, XMFLOAT2());
			}
		}

		// Stat Textures
		const int nStatIcons = 1; // Stat Window
		const int nPlusMinusIcons = 18; // +- Icons
		m_nExtraTextures = nStatIcons + nPlusMinusIcons;
		m_ppExtraTextures = new CUIObject * [m_nExtraTextures];

		CTexture* pStatTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/StatBox.dds");
		CTexture* pPlusTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Plus.dds");
		CTexture* pMinusTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Icon_Minus.dds");
		CScene::CreateShaderResourceViews(pd3dDevice, pStatTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, pPlusTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, pMinusTexture, 0, 3);


		for (int i = 0; i < m_nExtraTextures; ++i)
		{
			XMFLOAT2 rectSize = i < nStatIcons ? CalculateScreenResolutionSize(0.2704f, 0.375f) :
				CalculateScreenResolutionSize(0.02778f, 0.0375f);
			TEXTURETYPE type = i < nStatIcons ? TEXTURETYPE::NONE : TEXTURETYPE::BUTTON;
			CTexture* pTexture = i < nStatIcons ? pStatTexture :
				(i % 2 == 1 ? pPlusTexture : pMinusTexture);
			float texVal = 1.0f;
			XMFLOAT2 uvOffset = XMFLOAT2();
			XMFLOAT2 pos = i < nStatIcons ? CalculateScreenResolutionPos(0.65f, 0.5f) :
				CalculateScreenResolutionPos(0.8267f + (i-1) % 2 * 0.0375f, 0.50696f + 0.0375f * static_cast<int>((i-1)/2));
			m_ppExtraTextures[i] = new CUIObject(pd3dDevice, pd3dCommandList, pTexture, rectSize, pos, type, texVal, uvOffset);
			m_ppExtraTextures[i]->DrawOff();
		}

		// Set UI Buttons Functions... not basic function
		const int statButtonIndex = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 3 : 1;
		function<void()> statButtonClickCallback =
			[obj = m_ppUITextures[statButtonIndex], statObjs = m_ppExtraTextures, statObjNum = m_nExtraTextures]()
		{
			if (statObjs[0]->m_bIsRender) {
				for (int i = 0; i < statObjNum; ++i) {
					statObjs[i]->DrawOff();
				}
			}
			else {
				for (int i = 0; i < statObjNum; ++i) {
					statObjs[i]->DrawOn();
				}
			}
			obj->m_pMesh->SetValue(0.5f);
		};
		m_ppUITextures[statButtonIndex]->SetOnClickCallback(statButtonClickCallback);

		auto adjustStatsAndSetValue = [](CPlayer* player, CUIObject* obj, int statIndex, int statChange)
		{
			player->AdjustStats(statIndex, statChange);
			obj->m_pMesh->SetValue(0.5f);
		};

		std::vector<std::function<void()>> statFunctions;

		for (int i = 1; i <= m_nExtraTextures; ++i)
		{
			CPlayer* player = NetworkManager::GetInstance()->myClient;
			CUIObject* obj = m_ppExtraTextures[i];

			statFunctions.push_back([player, obj, i, &adjustStatsAndSetValue]()
				{
					int statIndex = (i + 1) / 2;
					int statChange = (i % 2 == 0) ? -1 : 1;

					adjustStatsAndSetValue(player, obj, statIndex, statChange);
				});
		}

		for (int i = 1; i < m_nExtraTextures; ++i)
			m_ppExtraTextures[i]->SetOnClickCallback(statFunctions[i - 1]);

		CTexture* pSkillInfoTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/SkillInfo.dds");
		CScene::CreateShaderResourceViews(pd3dDevice, pSkillInfoTexture, 0, 3);
		m_pSkillInfoTexture = new CUIObject(pd3dDevice, pd3dCommandList, pSkillInfoTexture,
			XMFLOAT2(393.0f, 208.0f), XMFLOAT2(), TEXTURETYPE::SKILLINFO, 1.0f, XMFLOAT2());
		m_pSkillInfoTexture->DrawOff();

		function<void()> hoverEndCallback = [obj = m_pSkillInfoTexture]() {obj->DrawOff(); };

		for (int i = 0; i < nSkillWindows; ++i) {
			function<void()> hoverCallback = [index = i]() {
				ORDER order = SceneManager::GetInstance()->GetOrder();
				CTextureShader::GetInstance()->SetSkillInfoTexture(order, index);
			};

			m_ppObjects[i]->SetOnHoverCallback(hoverCallback);
			m_ppObjects[i]->SetOnHoverEndCallback(hoverEndCallback);
		}

		break;
	}
	case SCENEKIND::INGAME:
	{
		// Player UI Textures Build
		m_nUITextures = 3;// HP, MP, Player Info
		m_ppUITextures = new CUIObject * [m_nUITextures];

		vector<CTexture*> ppTexturesUI(m_nUITextures);
		vector<XMFLOAT2> sizes = {
			CalculateScreenResolutionSize(0.26f, 0.02125f),
			CalculateScreenResolutionSize(0.26f, 0.02125f),
			CalculateScreenResolutionSize(0.8f, 0.135f)
		};
		vector<XMFLOAT2> positions = {
			CalculateScreenResolutionPos(0.1815f, 0.8875f),
			CalculateScreenResolutionPos(0.5556f, 0.8875f),
			CalculateScreenResolutionPos(0.1f, 0.865f)
		};
		vector<TEXTURETYPE> types = {
				TEXTURETYPE::PROGRESSBARR, TEXTURETYPE::PROGRESSBAR, // HP/MP Bars
				TEXTURETYPE::NONE	// Info GUI
		};

		int num = 0;

		ppTexturesUI[num++] = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Hp_Mana_bars_1_back.dds");
		ppTexturesUI[num++] = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Hp_Mana_bars_2_back.dds");
		ppTexturesUI[num++] = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/PlayerInfo_Bar.dds");

		for (int i = 0; i < m_nUITextures; ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, ppTexturesUI[i], 0, 3);
			m_ppUITextures[i] = new CUIObject(pd3dDevice, pd3dCommandList, ppTexturesUI[i], sizes[i], positions[i], types[i],
				1.0f, XMFLOAT2());
		}
		
		// Skill Icons Build
		const int numObjects = 4;
		m_nObjects = numObjects;
		m_ppObjects = new CUIObject * [m_nObjects];

		// Load skill icon textures
		const WCHAR* skillIconTexturePath = nullptr;
		if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
			skillIconTexturePath = L"Image/Player_SKillSet.dds";
		else
			skillIconTexturePath = L"Image/Boss_SkillSet.dds";

		CTexture* pTexture = LoadTexture(pd3dDevice, pd3dCommandList, skillIconTexturePath);
		CScene::CreateShaderResourceViews(pd3dDevice, pTexture, 0, 3);
		// Temporary Struct for skill Icon
		struct SkillIconProperties
		{
			int skillNumOffset;
			int numColumns;
			int numRows;
			float width;
			float height;
			float screenPositionXOffset;
		};
		SkillIconProperties skillIconProps;

		if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
		{
			skillIconProps = { -48, 12, 4, FRAME_BUFFER_RESIZE * 0.037f, FRAME_BUFFER_HEIGHT * 0.05f, 0.0415f };
		}
		else
		{
			skillIconProps = { -21, 10, 2, FRAME_BUFFER_RESIZE * 0.037f, FRAME_BUFFER_HEIGHT * 0.05f, 0.0415f };
		}

		array<int, 4> playerSkillNums = NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()];
		for (int i = 0; i < numObjects; ++i)
		{
			// Calculate skill icon UV coordinates
			int skillNum = playerSkillNums[i] + skillIconProps.skillNumOffset;
			int column = skillNum % skillIconProps.numColumns;
			int row = skillNum / skillIconProps.numColumns;
			float u = column * (1.0f / skillIconProps.numColumns);
			float v = row * (1.0f / skillIconProps.numRows);

			TEXTURETYPE type = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? TEXTURETYPE::PLAYERSKILL : TEXTURETYPE::BOSSSKILL;
			
			m_ppObjects[i] = new CUIObject(pd3dDevice, pd3dCommandList, pTexture,
				XMFLOAT2(skillIconProps.width, skillIconProps.height),
				XMFLOAT2(CalculateScreenResolutionPos(0.5715f + i * skillIconProps.screenPositionXOffset, 0.93625f)),
				type, 1.0f, XMFLOAT2(u, v));
		}


		m_nExtraTextures = NumMinimaps + NumNexus + NumTowers + NumTeleports + NumUnique + NumBlueCircles + NumRedCircles;
		m_ppExtraTextures = new CUIObject * [m_nExtraTextures];

		CTexture* textures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Minimap_Cartoon.dds"), // Minimap
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Nexus.dds"), // Nexus
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Tower.dds"), // Tower
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Teleport.dds"), // Teleport
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Unique_Icon.dds"), // Unique
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Blue_Circle.dds"), // Blue Circle
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Minimap/Red_Circle.dds") // Red Circle
		};

		int numIcons[] = { NumMinimaps, NumNexus, NumTowers, NumTeleports, NumUnique, NumBlueCircles, NumRedCircles };

		XMFLOAT2 towerPos[] = {XMFLOAT2(-68.47006f, -134.9801f), XMFLOAT2(-78.08006f, -103.5401f), 
			XMFLOAT2(-102.8901f, -78.76004f), XMFLOAT2(-129.9301f, -70.61005f) };
		
		XMFLOAT2 teleportPos[] = {XMFLOAT2(-97.50009f, -30.10003f), XMFLOAT2(-24.50001f, -103.5001f)};

		int currentIndex = 0;
		for (int i = 0; i < sizeof(numIcons) / sizeof(numIcons[0]); ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, textures[i], 0, 3);
			for (int j = 0; j < numIcons[i]; ++j)
			{
				XMFLOAT2 pos = XMFLOAT2();
				XMFLOAT2 size = i == 0 ? CalculateScreenResolutionSize(0.3204f, 0.4325f) :
					i == 5 || i == 6 ? CalculateScreenResolutionSize(0.01852f, 0.025f) :
					CalculateScreenResolutionSize(0.03148f, 0.0425f);

				switch (i)
				{
				case 0: pos = XMFLOAT2(FRAME_BUFFER_WIDTH * 0.75671875f, 0.0f); break;
				case 1: pos = WorldToMinimap(XMFLOAT2(-123.2651f, -127.6417f), size); break;
				case 2: pos = WorldToMinimap(towerPos[j], size); break;
				case 3: pos = WorldToMinimap(teleportPos[j], size); break;
				}
				m_ppExtraTextures[currentIndex] = new CUIObject(pd3dDevice, pd3dCommandList, textures[i],
					size, pos, TEXTURETYPE::MINIMAPICON, 1.0f, XMFLOAT2()); 
				currentIndex++;
			}
		}

		m_nShopTextures = NumShop + NumPurchaseLong + NumPurchaseShort + NumExit;
		CTexture* shopTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/Shop.dds"), // Shop
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/Purchase_Long.dds"), // Purchase Button Long
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/Purchase_Short.dds"), // Purchase Button Short
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Shop/Exit.dds"), // Exit Button
		};

		int numShopTextures[] = { NumShop, NumPurchaseLong, NumPurchaseShort, NumExit };

		for (int i = 0; i < sizeof(numShopTextures) / sizeof(numShopTextures[0]); ++i)
		{
			CScene::CreateShaderResourceViews(pd3dDevice, shopTextures[i], 0, 3);
			for (int j = 0; j < numShopTextures[i]; ++j)
			{
				XMFLOAT2 pos = XMFLOAT2();
				XMFLOAT2 size = i == 0 ? CalculateScreenResolutionSize(1.0f, 1.0f) :
					i == 1 ? CalculateScreenResolutionSize(0.09352f, 0.0425f) :
					i == 2 ? CalculateScreenResolutionSize(0.0463f, 0.0425f) : 
					CalculateScreenResolutionSize(0.09352f, 0.08375f);
				TEXTURETYPE type = i == 0 ? TEXTURETYPE::NONE : TEXTURETYPE::BUTTON;

				switch (i)
				{
				case 0: pos = XMFLOAT2(231.0f, 0.0f); break;
				case 1: // potions
				{
					pos = j < 5 ? CalculateScreenResolutionPos(0.213f, 0.285f + j * 0.08625f) :
						CalculateScreenResolutionPos(0.46667f, 0.285f + (j - 5) * 0.08625f);
					break;
				}
				case 2: // Stats
					pos = CalculateScreenResolutionPos(0.87315f, 0.2425f + j * 0.05925f);
					break;
				case 3: // exit
					pos = CalculateScreenResolutionPos(0.21204f, 0.67125f);
					break;
				}
				m_shopTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, shopTextures[i], size, pos, type, 1.0f, XMFLOAT2()));
			}
		}

		currentIndex = 0;
		function<void()> callback;
		for (int i = 0; i < sizeof(numShopTextures) / sizeof(numShopTextures[0]); ++i)
		{

			for (int j = 0; j < numShopTextures[i]; ++j)
			{
				switch (i)
				{
				case 1: // potions
				{
					int potionPrice = j < 2 ? 50 : 100;
					ITEMKIND potionItem = static_cast<ITEMKIND>(j);
					callback = [price = potionPrice, obj = m_shopTextures[currentIndex], item = potionItem]() {
						auto player = dynamic_cast<CGamePlayer*>(NetworkManager::GetInstance()->myClient);
						if (player->GetGold() >= price && !player->IsBagFull()) {
							NetworkManager::GetInstance()->SendBuyItemPacket(item);
							player->SetItem(item);
							SoundManager::GetInstance()->Play_Sound(L"ShopBuy.wav", CHANNELID::EFFECT, 0.7f);
						}
						else
							SoundManager::GetInstance()->Play_Sound(L"BuyFail.wav", CHANNELID::EFFECT, 0.5f);
						obj->m_pMesh->SetValue(0.5f);
					};
					break;
				}
				case 2: // Stats
					callback = [index = j, obj = m_shopTextures[currentIndex]]() {
						auto player = dynamic_cast<CGamePlayer*>(NetworkManager::GetInstance()->myClient);
						short gold = player->GetGold();
						int price = CTextureShader::GetInstance()->GetPrice(index);
						if (gold >= price) {
							NetworkManager::GetInstance()->SendBuyStatPacket(static_cast<ITEMKIND>(index + 2));
							player->StatLevelUp(index);
							SoundManager::GetInstance()->Play_Sound(L"ShopBuy.wav", CHANNELID::EFFECT, 0.7f);
						}
						else
							SoundManager::GetInstance()->Play_Sound(L"BuyFail.wav", CHANNELID::EFFECT, 0.5f);
						obj->m_pMesh->SetValue(0.5f);
					};

					break;
				case 3: // exit
					callback = [this, obj = m_shopTextures[currentIndex]]() {
						this->ShopSwitch();
						obj->m_pMesh->SetValue(0.5f);
					};
					break;
				}
				if (callback) m_shopTextures[currentIndex]->SetOnClickCallback(callback);
				m_shopTextures[currentIndex]->DrawOff();
				currentIndex++;
			}
		}
		
		CTexture* itemTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Item_Set.dds"); 
		CScene::CreateShaderResourceViews(pd3dDevice, itemTexture, 0, 3); 

		for (int i = 0; i < NumItem; ++i)
		{
			XMFLOAT2 pos = CalculateScreenResolutionPos(0.17593f + i * 0.04907f, 0.9375f);
			XMFLOAT2 size = CalculateScreenResolutionSize(0.0352f, 0.0475f);
			m_ItemTextures.push_back(new CUIObject(pd3dDevice, pd3dCommandList, itemTexture, size, pos, 
				TEXTURETYPE::ITEMKIND, 1.0f, XMFLOAT2()));
			m_ItemTextures[i]->DrawOff();
		}

		CTexture* accelTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Speed.dds");
		CTexture* damageTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Blood.dds");
		CTexture* winTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Clear/Victory.dds");
		CTexture* defeatTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Clear/Defeat.dds");

		CScene::CreateShaderResourceViews(pd3dDevice, accelTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, damageTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, winTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, defeatTexture, 0, 3);

		m_pAccelerateObject = new CUIObject(pd3dDevice, pd3dCommandList, accelTexture, XMFLOAT2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT),
			XMFLOAT2(0.0f, 0.0f), TEXTURETYPE::SCREENEFFECTSPEED, 1.0F, XMFLOAT2());
		m_pGetDamageObject = new CUIObject(pd3dDevice, pd3dCommandList, damageTexture, XMFLOAT2(FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT),
			XMFLOAT2(0.0f, 0.0f), TEXTURETYPE::SCREENEFFECTBLOOD, 1.0F, XMFLOAT2());
		m_pWinTexture = new CUIObject(pd3dDevice, pd3dCommandList, winTexture, CalculateScreenResolutionSize(1.0f, 1.0f),
			CalculateScreenResolutionPos(0.0f, 0.0f), TEXTURETYPE::SCREENEFFECTWIN, 1.0F, XMFLOAT2());
		m_pDefeatTexture = new CUIObject(pd3dDevice, pd3dCommandList, defeatTexture, CalculateScreenResolutionSize(1.0f, 1.0f),
			CalculateScreenResolutionPos(0.0f, 0.0f), TEXTURETYPE::SCREENEFFECTDEFEAT, 1.0F, XMFLOAT2());

		m_pAccelerateObject->DrawOff();
		m_pGetDamageObject->DrawOff();
		m_pWinTexture->DrawOff();
		m_pDefeatTexture->DrawOff();

		break;
	}
	}
	

}

XMFLOAT2 CTextureShader::WorldToMinimap(XMFLOAT2 pos, XMFLOAT2 iconSize)
{
	XMFLOAT2 minimapPos = XMFLOAT2(FRAME_BUFFER_WIDTH * 0.75671875f, 0.0f);
	XMFLOAT2 minimapSize = CalculateScreenResolutionSize(0.3204f, 0.4325f);

	XMFLOAT2 modifiedPos = XMFLOAT2();

	XMFLOAT2 worldRangeX = XMFLOAT2(-177.55f, 60.5f);
	XMFLOAT2 worldRangeY = XMFLOAT2(-179.0f, 51.0f);
	float worldWidth = worldRangeX.y - worldRangeX.x;
	float worldHeight = worldRangeY.y - worldRangeY.x;

	modifiedPos.x = ((pos.x - worldRangeX.x) / worldWidth) * minimapSize.x + minimapPos.x - iconSize.x * 0.5f;
	modifiedPos.y = (1-(pos.y - worldRangeY.x) / worldHeight) * minimapSize.y + minimapPos.y - iconSize.y * 0.5f;

	return modifiedPos;
}

bool CTextureShader::isRenderMinimap()
{
	if (m_ppExtraTextures)
		if (m_ppExtraTextures[0]->m_bIsRender) return true;
	return false;
}

void CTextureShader::MinimapSwitch()
{
	if (m_ppExtraTextures) {
		bool isRender = m_ppExtraTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nExtraTextures; ++i)
		{
			if(isRender) m_ppExtraTextures[i]->DrawOff();
			else if(!isRender) m_ppExtraTextures[i]->DrawOn();
		}
	}
}

void CTextureShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++)
			if (m_ppObjects[j]) {
				/*delete m_ppObjects[j];
				m_ppObjects[j] = nullptr;*/
				m_ppObjects[j]->Release();
			}
		delete[] m_ppObjects;
		m_ppObjects = nullptr;
		m_nObjects = 0;
	}

	if (m_ppUITextures)
	{
		for (int j = 0; j < m_nUITextures; j++) 
			if (m_ppUITextures[j]) {
				/*delete m_ppUITextures[j];
				m_ppUITextures[j] = nullptr;*/
				m_ppUITextures[j]->Release();
			}
		delete[] m_ppUITextures;
		m_ppUITextures = nullptr;
		m_nUITextures = 0;
	}	
	
	if (m_ppExtraTextures)
	{
		for (int j = 0; j < m_nExtraTextures; j++) 
			if (m_ppExtraTextures[j]) {
				/*delete m_ppExtraTextures[j];
				m_ppExtraTextures[j] = nullptr;*/
				m_ppExtraTextures[j]->Release();
			}
		delete[] m_ppExtraTextures;
		m_ppExtraTextures = nullptr;
		m_nExtraTextures = 0;
	}

	if (m_nShopTextures) {
		for (int i = 0; i < m_nShopTextures; ++i) {
			//delete m_shopTextures[i]; //hj modify 
			m_shopTextures[i]->Release();
		}
		m_shopTextures.clear();
		m_nShopTextures = 0;
		if (m_ItemTextures.size()) { 
			for (int i = 0; i < 5; ++i) {
				//delete m_ItemTextures[i];//hj modify 
				m_ItemTextures[i]->Release();
			}
			m_ItemTextures.clear();
		}
	}

	if (m_nChannelTextures) {
		for (int i = 0; i < m_nChannelTextures; ++i) {
			//delete m_channelTextures[i];
			m_channelTextures[i]->Release();
		}
		m_channelTextures.clear();
		m_nChannelTextures = 0;
	}

	if (m_nAuctionTextures) {
		for (int i = 0; i < m_nAuctionTextures; ++i) {
			//delete m_auctionTextures[i];
			m_auctionTextures[i]->Release();
		}
		m_auctionTextures.clear();
		m_nAuctionTextures = 0;
	}

	if (m_nBlockchainTextures) {
		for (int i = 0; i < m_nBlockchainTextures; ++i) {
			//delete m_nBlockchainTextures[i];
			m_blockchainTextures[i]->Release();
		}
		m_blockchainTextures.clear();
		m_nBlockchainTextures = 0;
	}

	if (m_nCustomizeTextures) {
		for (int i = 0; i < m_nCustomizeTextures; ++i) {
			//delete m_nBlockchainTextures[i];
			m_customizeTextures[i]->Release();
		}
		m_customizeTextures.clear();
		m_nCustomizeTextures = 0;
	}

	m_pHoveredObject = nullptr;

	if (m_pAccelerateObject) {
		m_pAccelerateObject->Release();
		m_pAccelerateObject = nullptr;
	}

	if (m_pGetDamageObject) {
		m_pGetDamageObject->Release();
		m_pGetDamageObject = nullptr;
	}

	if (m_pWinTexture) {
		m_pWinTexture->Release();
		m_pWinTexture = nullptr;
	}
	if (m_pDefeatTexture) {
		m_pDefeatTexture->Release();
		m_pDefeatTexture = nullptr;
	}
	if (m_pSkillInfoTexture) {
		m_pSkillInfoTexture->Release();
		m_pSkillInfoTexture = nullptr;
	}

	if (m_auctionInfo) {
		//delete m_auctionInfo;
		m_auctionInfo = nullptr;
	}
	if (m_blockChainInfo) {
		//delete m_auctionInfo;
		m_blockChainInfo = nullptr;
	}
}

void CTextureShader::PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap)
{
	CStandardShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->SetDescriptorHeaps(1, &DescriptorHeap);

	switch (m_CurScene)
	{
	case SCENEKIND::TITLE:
	{
		for (int j = 0; j < m_nObjects; j++)
		{
			if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
		break;
	}
	case SCENEKIND::LOBBY:
	{
		for (int j = 0; j < m_nObjects; j++)
		{
			if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
		for (int j = 0; j < m_nUITextures; j++)
		{
			if (m_ppUITextures[j]) m_ppUITextures[j]->Render(pd3dCommandList, pCamera);
		}
		for (int i = 0; i < m_nExtraTextures; ++i) {
			if (m_ppExtraTextures[i]) {
				m_ppExtraTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}
		for (int i = m_nChannelTextures - 1; i >= 0; --i) {
			if (m_channelTextures[i]) {
				m_channelTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}
		for (int i = 0; i < m_nShopTextures; ++i) {
			if (m_shopTextures[i]) {
				m_shopTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}

		// check auction
		if (IsAuction()) {
			//Calculation of Remaining Time
			for (int i = 0; i < 7; ++i) {
				if (NetworkManager::GetInstance()->lobbySceneInfo->auctionPageInfos[i].buyPrice == 0) {
					m_auctionInfo->closingTime[i] = 0;
					continue;
				}
				std::tm tm_deadLine = {};
				std::istringstream ss(NetworkManager::GetInstance()->lobbySceneInfo->auctionPageInfos[i].deadLine);
				ss >> std::get_time(&tm_deadLine, "%Y-%m-%d %H:%M:%S");
				std::chrono::time_point<std::chrono::system_clock> deadLineTimePoint = std::chrono::system_clock::from_time_t(std::mktime(&tm_deadLine));
				std::chrono::time_point<std::chrono::system_clock> currentTimePoint = std::chrono::system_clock::now();
				int remainingTime = std::chrono::duration_cast<std::chrono::minutes>(deadLineTimePoint - currentTimePoint).count();
				m_auctionInfo->closingTime[i] = remainingTime;
				int type = m_auctionInfo->productParts[i]; 
				int num = m_auctionInfo->productNum[i]; 
				XMFLOAT2 uvOffset = GetCustomizeUvOffset(type, num);
				int index = NumAuction + NumLeftIcon + NumRightIcon + NumCalc + NumMyPart + i;
				dynamic_cast<CTexturedRectMesh*>(m_auctionTextures[index]->m_pMesh)->SetUV(uvOffset);
			}

			for (int i = 0; i < 7; ++i) {
				if (m_auctionInfo->closingTime[i] == 0)
					AuctionProductOff(i);
				else AuctionProductOn(i);
			}
		}
		for (int i = 0; i < m_nAuctionTextures; ++i) {
			if (m_auctionTextures[i]) {
				m_auctionTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}
		for (int i = 0; i < m_nBlockchainTextures; ++i) {
			if (m_blockchainTextures[i]) {
				m_blockchainTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}
		for (int i = 0; i < m_nCustomizeTextures; ++i) {
			if (m_customizeTextures[i]) {
				m_customizeTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}
		break;
	}
	case SCENEKIND::READY:
	{
		for (int j = 0; j < m_nUITextures; j++)
		{
			if (m_ppUITextures[j]) m_ppUITextures[j]->Render(pd3dCommandList, pCamera);
		}

		const int nJobs = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 3 : 1;
		const int nSkillWindows = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 12 : 10;
		const int nSelectedSkills = SceneManager::GetInstance()->GetOrder() != ORDER::BOSS ? 4 * 3 : 4;
		ORDER curPlayer = SceneManager::GetInstance()->GetOrder();
		int job = curPlayer == ORDER::BOSS ? static_cast<int>(m_eBossJOB) * 10 :
			static_cast<int>(m_ePlayerJOB[static_cast<int>(curPlayer)]) * 12;
		int columnOffset = curPlayer == ORDER::BOSS ? 10 : 12;
		int rowOffset = curPlayer == ORDER::BOSS ? 2 : 4;
		
		for (int j = 0; j < m_nObjects; j++)
		{

			if (m_bReadySceneSKillUI && j < nSkillWindows)
			{
				int index = j + job;
				int column = index % columnOffset;
				int row = index / columnOffset;
				float u = column * (1.0f / columnOffset);
				float v = row * (1.0f / rowOffset);
				dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetUV(XMFLOAT2(u, v));
			}
			else if (j >= nSkillWindows && j < nSkillWindows + nSelectedSkills)
			{
				if (curPlayer == ORDER::BOSS)
				{
					int index = j - nSkillWindows;
					if (m_iPlayerSkillArrays[3][index])
					{
						int skillNum = m_iPlayerSkillArrays[3][index];
						int x = skillNum % columnOffset;
						int y = skillNum / columnOffset;
						float u = x * (1.0f / columnOffset);
						float v = y * (1.0f / rowOffset);
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetUV(XMFLOAT2(u, v));
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetValue(1.0f);
					}
					else
					{
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetValue(0.0f);
					}
				}
				else
				{
					int index = j - nSkillWindows;
					int column = index / 4; // 0, 1, 2
					int row = index % 4; // 0, 1, 2, 3
					if (m_iPlayerSkillArrays[column][row] != 0)
					{
						int skillNum = m_iPlayerSkillArrays[column][row];
						int x = skillNum % columnOffset;
						int y = skillNum / columnOffset;
						float u = x * (1.0f / columnOffset);
						float v = y * (1.0f / rowOffset);
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetUV(XMFLOAT2(u, v));
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetValue(1.0f);
					}
					else
					{
						dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetValue(0.0f);
					}
				}
			}
			else if (j >= nSkillWindows + nSelectedSkills)
			{
				int index = j - nSkillWindows - nSelectedSkills;
				int offset = curPlayer == ORDER::BOSS ? 4 : 0;
				int job = curPlayer == ORDER::BOSS ? static_cast<int>(m_eBossJOB) : static_cast<int>(m_ePlayerJOB[index]);
				float u = static_cast<float>((job + offset) / 6.0f);
				dynamic_cast<CTexturedRectMesh*>(m_ppObjects[j]->m_pMesh)->SetUV(XMFLOAT2(u, 0.f));
			}

			if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
		
		for (int i = 0; i < m_nExtraTextures; ++i) {
			if (m_ppExtraTextures[i]) {
				m_ppExtraTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}

		if (m_pSkillInfoTexture) {
			m_pSkillInfoTexture->Render(pd3dCommandList, pCamera);
		}

		break;
	}
	case SCENEKIND::INGAME:
	{
		float hpVal = NetworkManager::GetInstance()->myClient->GetHpPercentage();
		float mpVal = NetworkManager::GetInstance()->myClient->GetMpPercentage();
		//cout << "HP: " << hpVal << ", MP: " << mpVal << endl;

		m_ppUITextures[0]->m_pMesh->SetValue(hpVal);
		m_ppUITextures[1]->m_pMesh->SetValue(mpVal);

		for (int j = 0; j < m_nUITextures; j++)
		{
			if (m_ppUITextures[j]) m_ppUITextures[j]->Render(pd3dCommandList, pCamera);
		}

		for (int i = 0; i < m_nObjects; ++i) {
			if (m_ppObjects[i]) {
				m_ppObjects[i]->Render(pd3dCommandList, pCamera);
			}
		}		

		int numIcons[] = { NumMinimaps, NumNexus, NumTowers, NumTeleports, NumUnique, NumBlueCircles, NumRedCircles };
		int currentIndex = 0;
	 	
		XMFLOAT2 size = CalculateScreenResolutionSize(0.01852f, 0.025f);
		for (int i = 0; i < sizeof(numIcons) / sizeof(numIcons[0]); ++i)
		{
			for (int j = 0; j < numIcons[i]; ++j)
			{

				switch (i)
				{
				case 1:
				{
					bool active = NetworkManager::GetInstance()->structureInfo[4].active;
					float val = active ? 1.0f : 0.5f;
					m_ppExtraTextures[currentIndex]->m_pMesh->SetValue(val);
					break;
				}
				case 2:
				{
					bool active = NetworkManager::GetInstance()->structureInfo[j].active;
					float val = active ? 1.0f : 0.5f;
					m_ppExtraTextures[currentIndex]->m_pMesh->SetValue(val);
					break;
				}
				case 3:
				{
					bool active = NetworkManager::GetInstance()->teleportActive;
					float val = active ? 1.0f : 0.5f;
					m_ppExtraTextures[currentIndex]->m_pMesh->SetValue(val);
					break;
				}
				case 4:
				{
					bool isShow = NetworkManager::GetInstance()->monsterInfo[0].show;
					float val = isShow ? 1.0f : 0.0f;
					XMFLOAT2 monsterPos = XMFLOAT2(NetworkManager::GetInstance()->monsterInfo[0].x, NetworkManager::GetInstance()->monsterInfo[0].z);
					XMFLOAT2 pos = val == 1.0f ? WorldToMinimap(monsterPos, size) : XMFLOAT2(-999.0f, -999.0f);
					m_ppExtraTextures[currentIndex]->SetScreenPosition(pos);
					m_ppExtraTextures[currentIndex]->m_pMesh->SetValue(val); 
					break;
				}
				case 5: 
				{
					XMFLOAT3 playerPos = NetworkManager::GetInstance()->myClient->GetPosition();
					XMFLOAT3 otherPos = NetworkManager::GetInstance()->OtherClients[j]->GetPosition();
					float dx = otherPos.x - playerPos.x;
					float dz = otherPos.z - playerPos.z;
					float xzDistance = sqrt(dx * dx + dz * dz);
					int id = NetworkManager::GetInstance()->GetId();
					XMFLOAT2 pos = j == id ? XMFLOAT2(playerPos.x, playerPos.z) :
						XMFLOAT2(otherPos.x, otherPos.z);
					float val = j == id ? 1.5f : 1.0f;
					m_ppExtraTextures[currentIndex]->SetScreenPosition(WorldToMinimap(pos, size));
					m_ppExtraTextures[currentIndex]->m_pMesh->SetValue(val);

					bool isRender = m_ppExtraTextures[currentIndex]->m_bIsRender;
					if (SceneManager::GetInstance()->GetOrder() == ORDER::BOSS) {
						if ((xzDistance > 40.0f || !isRenderMinimap()) && isRender) m_ppExtraTextures[currentIndex]->DrawOff();
						else if (xzDistance <= 40.0f && !isRender && isRenderMinimap()) m_ppExtraTextures[currentIndex]->DrawOn();
					}
					break;
				}
				case 6:
				{
					 
					XMFLOAT3 playerPos = NetworkManager::GetInstance()->myClient->GetPosition();
					XMFLOAT3 otherPos = NetworkManager::GetInstance()->OtherClients[3]->GetPosition();
					float dx = otherPos.x - playerPos.x;
					float dz = otherPos.z - playerPos.z;
					float xzDistance = sqrt(dx * dx + dz * dz);
					int id = NetworkManager::GetInstance()->GetId();
					XMFLOAT2 pos = id == 3 ? XMFLOAT2(playerPos.x, playerPos.z) :
						XMFLOAT2(otherPos.x, otherPos.z);
					m_ppExtraTextures[currentIndex]->SetScreenPosition(WorldToMinimap(pos, size));
					bool isRender = m_ppExtraTextures[currentIndex]->m_bIsRender;
					if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) {
						if ((xzDistance > 40.0f || !isRenderMinimap()) && isRender) m_ppExtraTextures[currentIndex]->DrawOff();
						else if (xzDistance <= 40.0f && !isRender && isRenderMinimap()) m_ppExtraTextures[currentIndex]->DrawOn();
					}
					break;
				}
				}
				if (m_ppExtraTextures[currentIndex]) {
					m_ppExtraTextures[currentIndex]->Render(pd3dCommandList, pCamera);
				}
				currentIndex++;
			}
		}

		for (int i = 0; i < m_nExtraTextures; ++i) {
			if (m_ppExtraTextures[i]) {
				m_ppExtraTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}

		for (int i = 0; i < m_nShopTextures; ++i) {
			if (m_shopTextures[i]) {
				m_shopTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}

		for (int i = 0; i < 5; ++i) {
			if (m_ItemTextures[i]) {
				m_ItemTextures[i]->Render(pd3dCommandList, pCamera);
			}
		}

		if (m_pAccelerateObject) {
			m_pAccelerateObject->Render(pd3dCommandList, pCamera);
		}
		if (m_pGetDamageObject) {
			m_pGetDamageObject->Render(pd3dCommandList, pCamera);
		}
		if (m_pWinTexture) {
			m_pWinTexture->Render(pd3dCommandList, pCamera);
		}
		if (m_pDefeatTexture) {
			m_pDefeatTexture->Render(pd3dCommandList, pCamera);
		}

		break;
	}

	}
}

void CTextureShader::ReleaseUploadBuffers()
{
	CStandardShader::ReleaseUploadBuffers();
}

D3D12_SHADER_BYTECODE CTextureShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"UI.hlsl", "VSTextured", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CTextureShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"UI.hlsl", "PSTextured", "ps_5_1", &m_pd3dPixelShaderBlob));
}

int CTextureShader::SetSkillArray(int SkillNumber, ORDER ePos)
{
	int skillNumOffset = PLAYER_SKILL / 4;
	int maxSkill = 12;
	if (ePos == ORDER::BOSS) {
		skillNumOffset = BOSS_SKILL / 2;
		maxSkill = 10;
	}

	if (SkillNumber % maxSkill < 9) {
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				if (m_iPlayerSkillArrays[static_cast<int>(ePos)][j] == SkillNumber + skillNumOffset) {
					return j;
				}
			}
			if (m_iPlayerSkillArrays[static_cast<int>(ePos)][i] == 0) {
				m_iPlayerSkillArrays[static_cast<int>(ePos)][i] = SkillNumber + skillNumOffset;
				NetworkManager::GetInstance()->SendSkillSelectPacket(i, SkillNumber + skillNumOffset);
				return i;
			}
		}
	}
	else {
		if (m_iPlayerSkillArrays[static_cast<int>(ePos)][3] == SkillNumber + skillNumOffset) {
			return 3;
		}
		else if (m_iPlayerSkillArrays[static_cast<int>(ePos)][3] == 0) {
			m_iPlayerSkillArrays[static_cast<int>(ePos)][3] = SkillNumber + skillNumOffset;
			NetworkManager::GetInstance()->SendSkillSelectPacket(3, SkillNumber + skillNumOffset);
			return 3;
		}
	}
	return -1;
}

void CTextureShader::SetSkillArrayReset(int ArrayNum, ORDER ePos)
{
	int offset = ePos == ORDER::BOSS ? 10 : 12;
	dynamic_cast<CTexturedRectMesh*>(m_ppObjects[ArrayNum + offset]->m_pMesh)->SetValue(0.f);
	m_iPlayerSkillArrays[static_cast<int>(ePos)][ArrayNum] = 0;
	
	NetworkManager::GetInstance()->SendSkillSelectPacket(ArrayNum, 0);
}

void CTextureShader::SetSkillServer(array<int, 4> Skill, ORDER epos)
{
	for (int i = 0; i < 4; ++i) {
		m_iPlayerSkillArrays[static_cast<int>(epos)][i] = Skill[i];
	}
}

void CTextureShader::OnMouseMoved(float mouseX, float mouseY)
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY) {
		SetSkillInfoTexturePos(static_cast<int>(mouseX), static_cast<int>(mouseY));
	}

	if (m_pHoveredObject)
	{
		UIOBJECTSTATE state = m_pHoveredObject->GetOBJState();
		if(state != UIOBJECTSTATE::CLICK)
			m_pHoveredObject = m_pHoveredObject->OnMouseMoved(mouseX, mouseY);
	}
	else
	{
		switch (SceneManager::GetInstance()->m_nCurScene)
		{
		case SCENEKIND::TITLE:
		{
			MouseMoveLoop(m_ppObjects, m_nObjects, mouseX, mouseY);
			break;
		}

		case SCENEKIND::LOBBY:
		{
			if (IsSetting()) {
				MouseMoveLoop(m_ppExtraTextures, m_nExtraTextures, mouseX, mouseY);
			}
			else if (IsChannel())
			{
				MouseMoveLoop(m_channelTextures.data(), m_nChannelTextures, mouseX, mouseY);
			}
			else if (IsRandomShop()) 
			{
				MouseMoveLoop(m_shopTextures.data(), m_nShopTextures, mouseX, mouseY);
			}
			else if (IsAuction()) 
			{
				MouseMoveLoop(m_auctionTextures.data(), m_nAuctionTextures, mouseX, mouseY);
			}
			else if (IsBlockchain())
			{
				MouseMoveLoop(m_blockchainTextures.data(), m_nBlockchainTextures, mouseX, mouseY);
			}
			else if (IsCustomize())
			{
				MouseMoveLoop(m_customizeTextures.data(), m_nCustomizeTextures, mouseX, mouseY);
			}
			else {
				MouseMoveLoop(m_ppObjects, m_nObjects, mouseX, mouseY);
				MouseMoveLoop(m_ppUITextures, m_nUITextures, mouseX, mouseY);
			}
			break;
		}

		case SCENEKIND::READY:
		{
			MouseMoveLoop(m_ppObjects, m_nObjects, mouseX, mouseY);
			MouseMoveLoop(m_ppUITextures, m_nUITextures, mouseX, mouseY);
			if (isRenderStats()) {
				MouseMoveLoop(m_ppExtraTextures, m_nExtraTextures, mouseX, mouseY);
			}
			break;
		}

		case SCENEKIND::INGAME:
		{
			if (IsShopping()) {
				MouseMoveLoop(m_shopTextures.data(), m_nShopTextures, mouseX, mouseY);
			}

			break;
		}
		}
	}
}

void CTextureShader::OnMouseClick()
{
	if (m_pHoveredObject)
	{
		m_pHoveredObject->OnClick();
	}
}

void CTextureShader::OnMouseRelease()
{
	if (m_pHoveredObject)
	{
		m_pHoveredObject = m_pHoveredObject->OnRelease();
	}
}

bool CTextureShader::isRenderStats()
{
	if (m_ppExtraTextures)
		if (m_ppExtraTextures[0]->m_bIsRender) return true;
	return false;
}

bool CTextureShader::isRenderShop()
{
	if (m_nShopTextures)
		if (m_shopTextures[0]->m_bIsRender) return true;
	return false;
}

bool CTextureShader::ReadyPlayer(ORDER ePos)
{
	for (int i = 0; i < 4; ++i) {
		if (m_iPlayerSkillArrays[static_cast<int>(ePos)][i] == 0) {
			return false;
		}
	}
	return true;
}

int CTextureShader::GetPrice(int index)
{
	int price = 0;
	Stat_Level stats = dynamic_cast<CGamePlayer*>(NetworkManager::GetInstance()->myClient)->GetStats();

	switch (index)
	{
	case 0: price = stats.hp_price; break;
	case 1: price = stats.mp_price; break;
	case 2: price = stats.attack_price; break;
	case 3: price = stats.magic_attack_price; break;
	case 4: price = stats.defense_price; break;
	case 5: price = stats.magic_defense_price; break;
	case 6: price = stats.speed_price; break;
	case 7: price = stats.tenacity_price; break;
	case 8: price = stats.critical_price; break;
	}

	return price;
}

void CTextureShader::ShopSwitch()
{
	if (m_nShopTextures) {
		bool isRender = m_shopTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nShopTextures; ++i)
		{
			if (isRender) m_shopTextures[i]->DrawOff();
			else if (!isRender) m_shopTextures[i]->DrawOn();
		}
		m_shopping = !m_shopping;
	}
}

void CTextureShader::SetItemState(int index, bool state, ITEMKIND item)
{
	if (state) {
		float u = static_cast<int>(item) / 11.0f;
		dynamic_cast<CTexturedRectMesh*>(m_ItemTextures[index]->m_pMesh)->SetUV(XMFLOAT2(u, 0.0f));
		m_ItemTextures[index]->DrawOn();
	}
	else m_ItemTextures[index]->DrawOff();
}

void CTextureShader::SettingSwitch()
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nExtraTextures) {
		bool isRender = m_ppExtraTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nExtraTextures; ++i)
		{
			if (isRender) m_ppExtraTextures[i]->DrawOff();
			else if (!isRender) m_ppExtraTextures[i]->DrawOn();
		}
		m_setting = !m_setting;
	}
}

bool CTextureShader::IsSetting() const
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY) return m_setting;
	return true;
}

void CTextureShader::SettingReset()
{
	SceneManager::GetInstance()->m_nDrawOption = HABLE_MACCANN;
	m_ppExtraTextures[10]->SetScreenPosition(m_ppExtraTextures[4]->GetScreenPosition());

	SceneManager::GetInstance()->m_outline = true;
	m_ppExtraTextures[11]->SetScreenPosition(m_ppExtraTextures[6]->GetScreenPosition());

	SceneManager::GetInstance()->m_shadow = true;
	m_ppExtraTextures[12]->SetScreenPosition(m_ppExtraTextures[8]->GetScreenPosition());

	SceneManager::GetInstance()->m_fExposure = 1.1f;
	SceneManager::GetInstance()->m_fSaturation = 1.2f;
	SceneManager::GetInstance()->m_fContrast = 1.2f;
	SceneManager::GetInstance()->m_fVibrance = 1.6f;	

	m_masterVolume = 50;
	SoundManager::GetInstance()->ChangeVolume(VOLUME_INT_TO_FLOAT(m_masterVolume));
}

void CTextureShader::MouseMoveLoop(CUIObject** objects, int max, float mouseX, float mouseY)
{
	if (!m_pHoveredObject && max) {
		for (int i = 0; i < max; ++i)
		{
			m_pHoveredObject = objects[i]->OnMouseMoved(mouseX, mouseY);
			if (m_pHoveredObject) break;
		}
	}
}

void CTextureShader::ChannelSwitch()
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nChannelTextures) {
		bool isRender = m_channelTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nChannelTextures; ++i)
		{
			if (isRender) m_channelTextures[i]->DrawOff();
			else if (!isRender) m_channelTextures[i]->DrawOn();
		}
		m_channel = !m_channel;
	}
}

void CTextureShader::RandomShopSwitch() 
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nShopTextures) {
		m_pHoveredObject = nullptr;
		bool isRender = m_shopTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nShopTextures; ++i)
		{
			if (isRender) m_shopTextures[i]->DrawOff();
			else if (!isRender) m_shopTextures[i]->DrawOn();
		}
		m_randomShop = !m_randomShop;
		m_curPartsNum = 0;
	}
}

void CTextureShader::AddCurParts()
{
	if (!m_startRandom) {
		if (!m_shopTextures[5]->m_bIsRender)m_shopTextures[5]->DrawOn();
		m_curPartsNum++;
		if (m_curPartsNum > 26) m_curPartsNum = 0;

		short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;
		int yOffset = m_curPartsNum < 14 ? 13 - m_curPartsNum
			: sex == 0 ? 40 - m_curPartsNum
			: 53 - m_curPartsNum;

		float u = 0.0f;
		float v = static_cast<float>(yOffset) * (1.0f / 40.0f);
		XMFLOAT2 uvOffset = XMFLOAT2(u, v);
		dynamic_cast<CTexturedRectMesh*>(m_shopTextures[4]->m_pMesh)->SetUV(uvOffset);
	}
}

void CTextureShader::QuestionIconSwitch()
{
	bool isRender = m_shopTextures[5]->m_bIsRender;

	if (isRender) m_shopTextures[5]->DrawOff();
	else if (!isRender) m_shopTextures[5]->DrawOn();

}

void CTextureShader::SubtractCurParts()
{
	if (!m_startRandom) {
		if (!m_shopTextures[5]->m_bIsRender)m_shopTextures[5]->DrawOn();
		m_curPartsNum--;
		if (m_curPartsNum < 0) m_curPartsNum = 26;
		short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;
		int yOffset = m_curPartsNum < 14 ? 13 - m_curPartsNum
			: sex == 0 ? 40 - m_curPartsNum
			: 53 - m_curPartsNum;

		float u = 0.0f;
		float v = static_cast<float>(yOffset) * (1.0f / 40.0f);
		XMFLOAT2 uvOffset = XMFLOAT2(u, v);
		dynamic_cast<CTexturedRectMesh*>(m_shopTextures[4]->m_pMesh)->SetUV(uvOffset);
	}
}

bool CTextureShader::RandomGenParts(float elapsedTime)
{
	if (m_startRandom) {
		if(!m_shopTextures[5]->m_bIsRender)m_shopTextures[5]->DrawOn();
		short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;

		const vector<int> maxParts = {
			11, 4, 13, 38, 13, 15, 21, 21, 6, 6, 12, 11, 11, 3,
			22, 13, sex == 1 ? 7 : 10, 28, 20, 20, 18, 18,17, 17, 28, 19, 19
		};
		int yOffset = m_curPartsNum < 14 ? 13 - m_curPartsNum
			: sex == 0 ? 40 - m_curPartsNum
			: 53 - m_curPartsNum;

		if (m_randomTime > 0.0f) {
			m_curWonPart = Util::GenerateRandomInt(0, maxParts[m_curPartsNum] - 1);	

			float u = m_curWonPart * (1.0f / 38.0f);
			float v = static_cast<float>(yOffset) * (1.0f / 40.0f);
			XMFLOAT2 uvOffset = XMFLOAT2(u, v);
			dynamic_cast<CTexturedRectMesh*>(m_shopTextures[4]->m_pMesh)->SetUV(uvOffset);
			m_randomTime -= elapsedTime;

			return false;
		}
		else {
			m_curWonPart = NetworkManager::GetInstance()->lobbySceneInfo->partNum;

			m_randomTime = 0.8f;
			m_startRandom = false;

			float u = m_curWonPart * (1.0f / 38.0f);
			float v = static_cast<float>(yOffset) * (1.0f / 40.0f);
			XMFLOAT2 uvOffset = XMFLOAT2(u, v);
			dynamic_cast<CTexturedRectMesh*>(m_shopTextures[4]->m_pMesh)->SetUV(uvOffset);

			return true;
		}
	}
	return false;
	
}

void CTextureShader::RandomStart()
{
	if (!m_startRandom) UILayer::GetInstance()->GenRandomMent();
	m_startRandom = true;
}

XMFLOAT2 CTextureShader::GetCustomizeUvOffset(int kind, int num)
{
	short sex = NetworkManager::GetInstance()->myClient->m_CustomizeInfo.Chr_Sex;
	int yOffset = kind < 14 ? 13 - kind
		: sex == 0 ? 40 - kind
		: 53 - kind;
	float u = num * (1.0f / 38.0f);
	float v = yOffset * (1.0f / 40.0f);
	XMFLOAT2 uvOffset = XMFLOAT2(u, v);

	return uvOffset;
}

void CTextureShader::AuctionSwitch()
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nAuctionTextures) {
		m_pHoveredObject = nullptr;
		bool isRender = m_auctionTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nAuctionTextures; ++i)
		{
			if (isRender) m_auctionTextures[i]->DrawOff();
			else if (!isRender) m_auctionTextures[i]->DrawOn();
		}
		m_auction = !m_auction;
	}
}

void CTextureShader::AuctionProductOff(int n)
{
	int index = n + 20;
	if (m_auctionTextures[index])m_auctionTextures[index]->DrawOff();
}

void CTextureShader::AuctionProductOn(int n)
{
	int index = n + 20;
	if (m_auctionTextures[index])m_auctionTextures[index]->DrawOn();
}

void CTextureShader::BlockChainSwitch() 
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nBlockchainTextures) {
		m_pHoveredObject = nullptr;
		bool isRender = m_blockchainTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nBlockchainTextures; ++i)
		{
			if (isRender) m_blockchainTextures[i]->DrawOff();
			else if (!isRender) m_blockchainTextures[i]->DrawOn();
		}
		m_blockchain = !m_blockchain; 
	}
}

void CTextureShader::CustomizeSwitch()
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_nCustomizeTextures) {
		m_pHoveredObject = nullptr;
		bool isRender = m_customizeTextures[0]->m_bIsRender;
		for (int i = 0; i < m_nCustomizeTextures; ++i)
		{
			if (isRender) m_customizeTextures[i]->DrawOff();
			else if (!isRender) m_customizeTextures[i]->DrawOn();
		}

		for (int i = 0; i < m_nObjects; ++i)
		{
			if (!isRender) m_ppObjects[i]->DrawOff();
			else if (isRender) m_ppObjects[i]->DrawOn();
		}
		for (int i = 0; i < m_nUITextures; ++i)
		{
			if (!isRender) m_ppUITextures[i]->DrawOff();
			else if (isRender) m_ppUITextures[i]->DrawOn();
		}
		m_customize = !m_customize;
	}
}

void CTextureShader::SetSkillInfoTexture(ORDER order, int num)
{
	if (m_bReadySceneSKillUI) {
		int id = NetworkManager::GetInstance()->GetId();
		int job = order == ORDER::BOSS ? static_cast<int>(m_eBossJOB) + 4 : static_cast<int>(m_ePlayerJOB[id]);
		float u = static_cast<float>(num) / 12.0f;
		float v = static_cast<float>(job) / 6.0f;
		static_cast<CTexturedRectMesh*>(m_pSkillInfoTexture->m_pMesh)->SetUV(XMFLOAT2(u, v));
		m_pSkillInfoTexture->DrawOn();
	}
}

void CTextureShader::AnimateSpeedTexture(float elapsedTime)
{
	if (m_accelerate) {
		m_speedTime -= elapsedTime;
		if (m_speedTime < 0.0f) {
			m_speedTime = 0.1f;
			m_curSpeedUOffset++;
		}
		if (m_pAccelerateObject) {
			reinterpret_cast<CTexturedRectMesh*>(m_pAccelerateObject->m_pMesh)->SetUV(XMFLOAT2(m_curSpeedUOffset / 4.0f, 0.0f));
			m_pAccelerateObject->DrawOn();
		}
	}
	else {
		if (m_pAccelerateObject) {
			m_pAccelerateObject->DrawOff();
		}
	}
}

void CTextureShader::AnimateGetDamagedTexture(float elapsedTime)
{
	if (m_getDamaged) {
		m_getDamagedTextureLifetime -= elapsedTime;
		m_pGetDamageObject->DrawOn();
		if (m_getDamagedTextureLifetime < 0) {
			m_getDamaged = false;
			m_getDamagedTextureLifetime = 0.3f;
			m_pGetDamageObject->DrawOff();
		}
	}
}

void CTextureShader::SetSkillInfoTexturePos(int mouseX, int mouseY)
{
	if (m_pSkillInfoTexture) {
		m_pSkillInfoTexture->SetScreenPosition(XMFLOAT2(static_cast<float>(mouseX), static_cast<float>(mouseY - 208)));
	}
}

void CTextureShader::InteractionSwitch(bool interaction)
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY && m_ppUITextures) {
		if (interaction)m_ppUITextures[4]->DrawOn();
		else m_ppUITextures[4]->DrawOff();
	}
}

void CTextureShader::AwayFromNpc()
{
	if (IsRandomShop()) RandomShopSwitch();
	if (IsAuction()) AuctionSwitch();
	if (IsBlockchain()) BlockChainSwitch();
	if (IsCustomize()) CustomizeSwitch();
}

int CTextureShader::GetStatLevel(int stat)
{
	int level = 0;
	Stat_Level stats = dynamic_cast<CGamePlayer*>(NetworkManager::GetInstance()->myClient)->GetStats();
	switch (stat)
	{
	case 0: level = stats.hp; break;
	case 1: level = stats.mp; break;
	case 2: level = stats.attack; break;
	case 3: level = stats.magic_attack; break;
	case 4: level = stats.defense; break;
	case 5: level = stats.magic_defense; break;
	case 6: level = stats.speed; break;
	case 7: level = stats.tenacity; break;
	case 8: level = stats.critical; break;
	}

	return level;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CDebugNormalShader::CDebugNormalShader()
{
}

CDebugNormalShader::~CDebugNormalShader()
{
}

void CDebugNormalShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_DEPTH_STENCIL_DESC CDebugNormalShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CDebugNormalShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "VSTextureToViewport", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CDebugNormalShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "PSTextureToViewport", "ps_5_1", &m_pd3dPixelShaderBlob));
}


void CDebugNormalShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}

CDebugDiffuseShader::CDebugDiffuseShader()
{
}

CDebugDiffuseShader::~CDebugDiffuseShader()
{
}

void CDebugDiffuseShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_DEPTH_STENCIL_DESC CDebugDiffuseShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CDebugDiffuseShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "VSDebugDiffuseToViewport", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CDebugDiffuseShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "PSDebugDiffuseToViewport", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CDebugDiffuseShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}

CDebugTextureShader::CDebugTextureShader()
{
}

CDebugTextureShader::~CDebugTextureShader()
{
}

void CDebugTextureShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_DEPTH_STENCIL_DESC CDebugTextureShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CDebugTextureShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "VSDebugTextureToViewport", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CDebugTextureShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "PSDebugTextureToViewport", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CDebugTextureShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}

CDebugLightingShader::CDebugLightingShader()
{
}

CDebugLightingShader::~CDebugLightingShader()
{
}

void CDebugLightingShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_DEPTH_STENCIL_DESC CDebugLightingShader::CreateDepthStencilState(int nPipelineState)
{
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = FALSE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

D3D12_SHADER_BYTECODE CDebugLightingShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "VSDebugLightingToViewport", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CDebugLightingShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "PSDebugLightingToViewport", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CDebugLightingShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pd3dCommandList->DrawInstanced(6, 1, 0, 0);
}

CBoundingBoxShader::CBoundingBoxShader()
{
}

CBoundingBoxShader::~CBoundingBoxShader()
{
}

D3D12_RASTERIZER_DESC CBoundingBoxShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_INPUT_LAYOUT_DESC CBoundingBoxShader::CreateInputLayout()
{
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_SHADER_BYTECODE CBoundingBoxShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "VSBoxTextured", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CBoundingBoxShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Debug.hlsl", "PSBoxTextured", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CBoundingBoxShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

void CBoundingBoxShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{
	m_nObjects = 1;

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2DARRAY, 0, 1);
	pTexture->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, L"Image/Test/StonesArray.dds", RESOURCE_TEXTURE2DARRAY, 0);

	CreateCbvSrvDescriptorHeaps(pd3dDevice, m_nObjects, 1);
	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	CreateShaderResourceViews(pd3dDevice, pTexture, 0, 3);

	m_pMaterial = new CMaterial(1);
	m_pMaterial->SetTexture(pTexture);

	m_ppObjects = new CGameObject * [m_nObjects];	

	for (int i = 0; i < m_nObjects; ++i)
	{
		CBoundingBoxTexturedMesh* pCubeMesh = new CBoundingBoxTexturedMesh(pd3dDevice, pd3dCommandList, 1, 1, 1);
		CBBObject* pRotatingObject = NULL;
		pRotatingObject = new CBBObject(1);
		pRotatingObject->SetMesh(pCubeMesh);
		
		pRotatingObject->SetScale(0.290938f * 2.f * 2.f, 0.888273f * 2, 0.2125f * 2.f * 8.f);
		// Player Bounding Box : 0.290938, 0.888273, 0.2125
		pRotatingObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
		m_ppObjects[i] = pRotatingObject;

	}
}

void CBoundingBoxShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) delete m_ppObjects[j];
		delete[] m_ppObjects;
	}

	if (m_pMaterial) m_pMaterial->Release();
}

void CBoundingBoxShader::ReleaseUploadBuffers()
{
}

void CBoundingBoxShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CStandardShader::Render(pd3dCommandList, pCamera);

	for (int j = 0; j < m_nObjects; j++)
	{
		//if (!m_ppObjects[j]->m_pSkinnedAnimationController) m_ppObjects[j]->UpdateTransform(NULL);
		if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
	}
}

void CBoundingBoxShader::Update(CPlayer* player)
{
	XMFLOAT3 look = player->GetLook(); look.y = 0;
	XMFLOAT3 offset = Vector3::ScalarProduct(look, 2.0f, false);
	offset.y += 0.908f;
	XMFLOAT3 playerPosition = player->GetPosition();
	XMFLOAT3 newPosition = XMFLOAT3(playerPosition.x + offset.x, playerPosition.y + offset.y, playerPosition.z + offset.z);
	m_ppObjects[0]->SetPosition(newPosition);

	XMMATRIX worldMatrix = XMLoadFloat4x4(&m_ppObjects[0]->m_xmf4x4World);
	XMVECTOR scale, rotation, translation;
	DirectX::XMMatrixDecompose(&scale, &rotation, &translation, worldMatrix);

	XMMATRIX playerWorldMatrix = XMLoadFloat4x4(&player->m_xmf4x4World);
	
	playerWorldMatrix.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

	XMMATRIX newWorldMatrix = DirectX::XMMatrixScalingFromVector(scale) * playerWorldMatrix * DirectX::XMMatrixTranslationFromVector(XMLoadFloat3(&newPosition));
	XMStoreFloat4x4(&m_ppObjects[0]->m_xmf4x4World, newWorldMatrix);

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CBlendApplyShader::CBlendApplyShader()
{
}

CBlendApplyShader::~CBlendApplyShader()
{
}

void CBlendApplyShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	CShader::CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, nPipelineState);
}

void CBlendApplyShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CStandardShader::Render(pd3dCommandList, pCamera);
}

D3D12_RASTERIZER_DESC CBlendApplyShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_SHADER_BYTECODE CBlendApplyShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Standard.hlsl", "VSStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CBlendApplyShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"FowardRender.hlsl", "PSBlend", "ps_5_1", &m_pd3dPixelShaderBlob));
}

//////////////////////////////////////////////////////////////////////////

CBlendObjectShader::CBlendObjectShader()
{
}

CBlendObjectShader::~CBlendObjectShader()
{
}

void CBlendObjectShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	CShader::CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, nPipelineState);
}

void CBlendObjectShader::PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap)
{
	pd3dCommandList->SetDescriptorHeaps(1, &DescriptorHeap);

	//CSkinnedAnimationObjectsShader::Render(pd3dCommandList, pCamera);

	CSkinnedAnimationStandardShader::Render(pd3dCommandList, pCamera);

	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j])
		{
			m_ppObjects[j]->Animate(m_fElapsedTime);
			m_ppObjects[j]->Render(pd3dCommandList, pCamera);
		}
	}

	//RenderManager::GetInstance()->OrderReder(pd3dCommandList, pCamera);
}

D3D12_RASTERIZER_DESC CBlendObjectShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_SHADER_BYTECODE CBlendObjectShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Standard.hlsl", "VSStandard", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CBlendObjectShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"FowardRender.hlsl", "PSBlend", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CBlendObjectShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{

	m_nObjects = 1;

	m_ppObjects = new CGameObject * [m_nObjects];

	CLoadedModelInfo* pTree = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/BlendObjectsForLobby2.bin", NULL);
	m_ppObjects[0] = new CBlendObject(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pTree, 0);
	m_ppObjects[0]->SetPosition(0.0f, 0.0f, 0.0f);
	m_ppObjects[0]->SetScale(1.f, 1.f, 1.f);
	m_ppObjects[0]->SetObjectID(1);
	m_ppObjects[0]->SetObjectType(OBJ_TYPE::TEXTURE); //Temporary

	CBlendApplyShader* pBlendShader = new CBlendApplyShader();
	pBlendShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature);
	pBlendShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	m_ppObjects[0]->SetRootShader(pBlendShader); 

	//RenderManager::GetInstance()->AddRenderVector(m_ppObjects[0]);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);
	if (pTree) delete pTree;

}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CBillboardUIShader::CBillboardUIShader()
{
}

CBillboardUIShader::CBillboardUIShader(CGameObject** ppMonsters, CGameObject** ppMinions, CGameObject** ppOtherClients)
{
	monsters = ppMonsters;
	minions = ppMinions;
	otherclients = ppOtherClients;
}

CBillboardUIShader::~CBillboardUIShader()
{
}

void CBillboardUIShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 1;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	// Deffered Rendering
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader();
	m_d3dPipelineStateDesc.PS = CreatePixelShader();
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	m_d3dPipelineStateDesc.NumRenderTargets = DEFERREDNUM + 1;
	for (UINT i = 0; i < DEFERREDNUM + 1; i++)
		m_d3dPipelineStateDesc.RTVFormats[i] = deferredBufferFormats[i];
	m_d3dPipelineStateDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[0]);

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;

}

D3D12_INPUT_LAYOUT_DESC CBillboardUIShader::CreateInputLayout()
{
	UINT nInputElementDescs = 2;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_RASTERIZER_DESC CBillboardUIShader::CreateRasterizerState(int nPipelineState)
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	//d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_BLEND_DESC CBillboardUIShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = TRUE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

void CBillboardUIShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{
	switch (SceneManager::GetInstance()->m_nCurScene)
	{
	case SCENEKIND::TITLE:
	{
		break;
	}
	case SCENEKIND::LOBBY:
	{
		m_nObjects = LOBBY_NPC;
		m_ppObjects = new CGameObject * [m_nObjects];

		CTexture* pTextures[] = {
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/ShopName.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/AuctionName.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/BlockChainName.dds"),
			LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/Lobby/CustomizeName.dds")
		};

		std::vector<CTexturedRectMesh*> ppNameMesh(m_nObjects); 
		std::vector<CMaterial*> ppTextureMaterials(m_nObjects);

		const XMFLOAT3 npcPositions[LOBBY_NPC] = {
			XMFLOAT3(2.552067f, 2.637576f, -6.728072f),
			XMFLOAT3(-19.102850f, 2.360098f,23.500757f),
			XMFLOAT3(-16.562815f,6.315217f,-19.624289f),
			XMFLOAT3(-19.408022f, 3.0709f, -57.537098f)
		};

		for (int i = 0; i < m_nObjects; ++i) {
			XMFLOAT3 pos = npcPositions[i];
			pos.y += i == 2 ? 4.0f : 3.0f;

			CScene::CreateShaderResourceViews(pd3dDevice, pTextures[i], 0, 3);
			ppNameMesh[i] = CreateTexturedRectMesh(pd3dDevice, pd3dCommandList, FRAME_BUFFER_RESIZE * 0.004f, FRAME_BUFFER_HEIGHT * 0.0016f);
			ppNameMesh[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
			ppTextureMaterials[i] = new CMaterial(1);
			ppTextureMaterials[i]->SetTexture(pTextures[i]);
			CGameObject* pObject = new CGameObject(1);
			ppNameMesh[i]->SetValue(1.0f);
			ppNameMesh[i]->SetType(TEXTURETYPE::NONE);
			pObject->SetMesh(ppNameMesh[i]);
			pObject->SetMaterial(0, ppTextureMaterials[i]); 
			pObject->SetObjectType(OBJ_TYPE::TEXTURE); 
			pObject->SetPosition(XMFLOAT3(pos));
			pObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList); 
			m_ppObjects[i] = pObject;
		}

		break;
	}
	case SCENEKIND::READY:
	{
		break;
	}
	case SCENEKIND::INGAME:
	{
		constexpr int numMonsters = MONSTER_NUM;
		constexpr int numClients = LOBBY_MAX_CLIENT;
		constexpr int numMinions = MAX_MINION;
		constexpr int numTowers = 4;
		constexpr int numNexus = 1;

		m_nObjects = numMonsters + numClients + numMinions + numTowers + numNexus;
		m_ppObjects = new CGameObject * [m_nObjects];
		m_ppOverlapTextures = new CGameObject * [m_nObjects];

		std::vector<CTexturedRectMesh*> ppHPMesh(m_nObjects);
		std::vector<CMaterial*> ppTextureMaterials(m_nObjects);

		std::vector<CTexturedRectMesh*> ppOverlapHPMesh(m_nObjects);
		std::vector<CMaterial*> ppOverlapTextureMaterials(m_nObjects);

		CTexture* hpBarRed = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/billboard/HP_bar_filling_red.dds");
		CTexture* hpBarGreen = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/billboard/HP_bar_filling_green.dds");
		CTexture* hpBarFrame = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/billboard/HP_bar_frame.dds");

		CScene::CreateShaderResourceViews(pd3dDevice, hpBarRed, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, hpBarGreen, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, hpBarFrame, 0, 3);

		for (int i = 0; i < numMonsters + numClients + numMinions; ++i)
		{
			float overlapWidth = i == 0 ? 0.006144f : i <= 2 ? 0.004096f : i < numMonsters + numClients ? 0.00256f : 0.001536f;
			float overlapHeight = i <= numMonsters + numClients ? 0.0004972f : 0.0002486f;
			float width = i == 0 ? 0.006f : i <= 2 ? 0.004f : i < numMonsters + numClients ? 0.0025f : 0.0015f;
			float height = i <= numMonsters + numClients ? 0.0004f : 0.0002f;

			CTexture* texture = i < numMonsters ? hpBarRed :
				i < numMonsters + numClients ?
				(((SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) && i - numMonsters < 3) ? hpBarGreen : hpBarRed) :
				(SceneManager::GetInstance()->GetOrder() == ORDER::BOSS) ? hpBarGreen : hpBarRed;

			ppHPMesh[i] = CreateTexturedRectMesh(pd3dDevice, pd3dCommandList, FRAME_BUFFER_RESIZE * width, FRAME_BUFFER_HEIGHT * height);
			ppOverlapHPMesh[i] = CreateTexturedRectMesh(pd3dDevice, pd3dCommandList, FRAME_BUFFER_RESIZE * overlapWidth, 
				FRAME_BUFFER_HEIGHT * overlapHeight);


			ppHPMesh[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
			ppTextureMaterials[i] = new CMaterial(1);
			ppTextureMaterials[i]->SetTexture(texture);

			ppOverlapHPMesh[i]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
			ppOverlapTextureMaterials[i] = new CMaterial(1);
			ppOverlapTextureMaterials[i]->SetTexture(hpBarFrame);

			CGameObject* pObject = new CGameObject(1);
			ppHPMesh[i]->SetValue(1.0);
			ppHPMesh[i]->SetType(TEXTURETYPE::PROGRESSBAR);
			pObject->SetMesh(ppHPMesh[i]);
			pObject->SetMaterial(0, ppTextureMaterials[i]);
			pObject->SetObjectType(OBJ_TYPE::TEXTURE);
			pObject->SetPosition(XMFLOAT3());
			pObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
			m_ppObjects[i] = pObject;

			CGameObject* pOverlapObject = new CGameObject(1);
			ppOverlapHPMesh[i]->SetValue(1.0);
			ppOverlapHPMesh[i]->SetType(TEXTURETYPE::NONE);
			pOverlapObject->SetMesh(ppOverlapHPMesh[i]);
			pOverlapObject->SetMaterial(0, ppOverlapTextureMaterials[i]);
			pOverlapObject->SetObjectType(OBJ_TYPE::TEXTURE);
			pOverlapObject->SetPosition(XMFLOAT3());
			pOverlapObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList);
			m_ppOverlapTextures[i] = pOverlapObject;
		}

		CTexture* towerBgTexture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/billboard/Experience_frame.dds");
		CTexture* texture = LoadTexture(pd3dDevice, pd3dCommandList, L"Image/GUI/billboard/HP_bar_filling_blue.dds");
		CScene::CreateShaderResourceViews(pd3dDevice, towerBgTexture, 0, 3);
		CScene::CreateShaderResourceViews(pd3dDevice, texture, 0, 3);
		int index = numMonsters + numClients + numMinions;
		for (; index < m_nObjects; ++index) {
			int i = index - numMonsters - numClients - numMinions;
			XMFLOAT3 pos = i < 4 ? TOWER_POS[i] : XMFLOAT3(-123.2651f, 10.700449f, -127.6417f);
			pos.y += 2.0f;
			XMFLOAT2 size = i < 4 ? CalculateScreenResolutionSize(0.003f, 0.0004f) : CalculateScreenResolutionSize(0.0036f, 0.00048f);
			ppHPMesh[index] = CreateTexturedRectMesh(pd3dDevice, pd3dCommandList, size.x, size.y);
			size = i < 4 ? CalculateScreenResolutionSize(0.003072f, 0.0004972f)
				: CalculateScreenResolutionSize(0.0036864f, 0.00059664f);
			ppOverlapHPMesh[index] = CreateTexturedRectMesh(pd3dDevice, pd3dCommandList, size.x, size.y);
			ppHPMesh[index]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
			ppTextureMaterials[index] = new CMaterial(1);
			ppTextureMaterials[index]->SetTexture(texture);

			ppOverlapHPMesh[index]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
			ppOverlapTextureMaterials[index] = new CMaterial(1);
			ppOverlapTextureMaterials[index]->SetTexture(towerBgTexture);

			CGameObject* pObject = new CGameObject(1);
			ppHPMesh[index]->SetValue(1.0);
			ppHPMesh[index]->SetType(TEXTURETYPE::PROGRESSBAR);
			pObject->SetMesh(ppHPMesh[index]);
			pObject->SetMaterial(0, ppTextureMaterials[index]);
			pObject->SetObjectType(OBJ_TYPE::TEXTURE); 
			pObject->SetPosition(pos);
			pObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList); 
			m_ppObjects[index] = pObject; 

			CGameObject* pOverlapObject = new CGameObject(1); 
			ppOverlapHPMesh[index]->SetValue(1.0);
			ppOverlapHPMesh[index]->SetType(TEXTURETYPE::NONE);
			pOverlapObject->SetMesh(ppOverlapHPMesh[index]);
			pOverlapObject->SetMaterial(0, ppOverlapTextureMaterials[index]);
			pOverlapObject->SetObjectType(OBJ_TYPE::TEXTURE); 
			pOverlapObject->SetPosition(pos);
			pOverlapObject->AllCreateShaderVariables(pd3dDevice, pd3dCommandList); 
			m_ppOverlapTextures[index] = pOverlapObject; 
		}

		break;
	}
	}

	
}

void CBillboardUIShader::ReleaseObjects()
{
	if (m_ppObjects)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppObjects[j]) delete m_ppObjects[j];
		delete[] m_ppObjects;
	}
	if (m_ppOverlapTextures)
	{
		for (int j = 0; j < m_nObjects; j++) if (m_ppOverlapTextures[j]) delete m_ppOverlapTextures[j];
		delete[] m_ppOverlapTextures;
	}
}

void CBillboardUIShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CStandardShader::Render(pd3dCommandList, pCamera);

	XMFLOAT3 xmf3CameraPosition = pCamera->GetPosition();
	for (int j = 0; j < m_nObjects; j++)
	{
		if (m_ppObjects[j]) m_ppObjects[j]->SetLookAt(xmf3CameraPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
		if (m_ppObjects[j]) m_ppObjects[j]->Render(pd3dCommandList, pCamera);
	}
}

void CBillboardUIShader::PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap)
{
	CStandardShader::Render(pd3dCommandList, pCamera);

	pd3dCommandList->SetDescriptorHeaps(1, &DescriptorHeap);

	XMFLOAT3 xmf3CameraPosition = pCamera->GetPosition();

	switch (SceneManager::GetInstance()->m_nCurScene)
	{
	case SCENEKIND::TITLE:
	{
		break;
	}
	case SCENEKIND::LOBBY:
	{
		for (int i = 0; i < m_nObjects; ++i) {
			if (m_ppObjects[i]) {
				m_ppObjects[i]->SetLookAt(xmf3CameraPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
				m_ppObjects[i]->Render(pd3dCommandList, pCamera);
			}
		}
		break;
	}
	case SCENEKIND::READY:
	{
		break;
	}
	case SCENEKIND::INGAME:
	{
		for (int j = 0; j < m_nObjects; j++)
		{
			bool isRender = true;
			if (m_ppObjects[j] && m_ppOverlapTextures[j])
			{
				if (j < MONSTER_NUM)
				{
					if (NetworkManager::GetInstance()->monsterInfo[j].show) {
						XMFLOAT3 pos = monsters[j]->GetPosition();
						pos.y += m_fBillboardOffset[j];
						m_ppObjects[j]->SetPosition(pos);
						m_ppOverlapTextures[j]->SetPosition(pos);
						m_ppObjects[j]->m_pMesh->SetValue(monsters[j]->GetHpPercentage());
					}
					else isRender = false;
				}
				else if (j < MONSTER_NUM + LOBBY_MAX_CLIENT)
				{
					int index = j - MONSTER_NUM;
					XMFLOAT3 pos = otherclients[index]->GetPosition();
					pos.y += m_fBillboardOffset[j];
					m_ppObjects[j]->SetPosition(pos);
					m_ppOverlapTextures[j]->SetPosition(pos);
					m_ppObjects[j]->m_pMesh->SetValue(reinterpret_cast<CPlayerObject*>(otherclients[index])->GetHpPercentage());
				}
				else if (j < MONSTER_NUM + LOBBY_MAX_CLIENT + MAX_MINION)
				{
					int index = j - MONSTER_NUM - LOBBY_MAX_CLIENT;
					if (minions[index]->m_bIsRender && NetworkManager::GetInstance()->npcInfo[index].show)
					{
						XMFLOAT3 pos = minions[index]->GetPosition();
						pos.y += m_fBillboardOffset[j];
						m_ppObjects[j]->SetPosition(pos);
						m_ppOverlapTextures[j]->SetPosition(pos);
						m_ppObjects[j]->m_pMesh->SetValue(minions[index]->GetHpPercentage());
					}
					else
						isRender = false;
				}
				else if (j < m_nObjects)
				{
					int index = j - MONSTER_NUM - LOBBY_MAX_CLIENT - MAX_MINION;
					if (!NetworkManager::GetInstance()->structureInfo[index].broken) 
					{
						XMFLOAT3 pos = XMFLOAT3();
						XMFLOAT3 playerPos = NetworkManager::GetInstance()->myClient->GetPosition();
						XMVECTOR playerPosVec = XMLoadFloat3(&playerPos);
						XMFLOAT3 nexusPos = XMFLOAT3(-123.2651f, 10.700449f, -127.6417f);
						XMVECTOR towerPosVec = index < 4 ? XMLoadFloat3(&TOWER_POS[index]) : XMLoadFloat3(&nexusPos);
						XMVECTOR diffVec = XMVectorSubtract(playerPosVec, towerPosVec);
						float distance = XMVectorGetX(XMVector3Length(diffVec));
						if (distance < 13.0f) {
							XMVECTOR directionVec = XMVector3Normalize(diffVec);
							XMVECTOR newPosVec = XMVectorMultiplyAdd(directionVec, XMVectorReplicate(8.0f), towerPosVec);
							XMStoreFloat3(&pos, newPosVec);
							pos.y = 5.0f;
							m_ppObjects[j]->SetPosition(pos);
							m_ppOverlapTextures[j]->SetPosition(pos);
						}
						else {
							XMFLOAT3 pos = index < 4 ? TOWER_POS[index] : XMFLOAT3(-123.2651f, 10.700449f, -127.6417f);
							pos.y += 2.0f;
							m_ppObjects[j]->SetPosition(pos);
							m_ppOverlapTextures[j]->SetPosition(pos);
						}
						float val = static_cast<float>(NetworkManager::GetInstance()->structureInfo[index].curHp) /
							NetworkManager::GetInstance()->structureInfo[index].maxHp;
						m_ppObjects[j]->m_pMesh->SetValue(val);
					}
					else
						isRender = false;
				}

				if (isRender) {
					m_ppObjects[j]->SetLookAt(xmf3CameraPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
					m_ppObjects[j]->Render(pd3dCommandList, pCamera);

					m_ppOverlapTextures[j]->SetLookAt(xmf3CameraPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
					m_ppOverlapTextures[j]->Render(pd3dCommandList, pCamera);
				}
			}
		}
		break;
	}
	}
	
}

void CBillboardUIShader::ReleaseUploadBuffers()
{
	CStandardShader::ReleaseUploadBuffers();
}

D3D12_SHADER_BYTECODE CBillboardUIShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Billboard.hlsl", "VSBillboard", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CBillboardUIShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Billboard.hlsl", "PSBillboard", "ps_5_1", &m_pd3dPixelShaderBlob));
}

/// <summary>
/// ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// </summary>

CParticleShader::CParticleShader()
{
}

CParticleShader::~CParticleShader()
{
}

D3D12_PRIMITIVE_TOPOLOGY_TYPE CParticleShader::GetPrimitiveTopologyType(int nPipelineState)
{
	return(D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT);
}

UINT CParticleShader::GetNumRenderTargets(int nPipelineState)
{
	return((nPipelineState == 0) ? 0 : 1);
}

DXGI_FORMAT CParticleShader::GetRTVFormat(int nPipelineState, int nRenderTarget)
{
	return((nPipelineState == 0) ? DXGI_FORMAT_UNKNOWN : DXGI_FORMAT_R8G8B8A8_UNORM);
}

DXGI_FORMAT CParticleShader::GetDSVFormat(int nPipelineState)
{
	return(DXGI_FORMAT_D32_FLOAT);
}

D3D12_SHADER_BYTECODE CParticleShader::CreateVertexShader(int nPipelineState)
{
	if (nPipelineState == 0)
		return(CompileShaderFromFile(L"Particle.hlsl", "VSParticleStreamOutput", "vs_5_1", &m_pd3dVertexShaderBlob));
	else
		return(CompileShaderFromFile(L"Particle.hlsl", "VSParticleDraw", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CParticleShader::CreateGeometryShader(int nPipelineState)
{
	if (nPipelineState == 0)
		return(CompileShaderFromFile(L"Particle.hlsl", "GSParticleStreamOutput", "gs_5_1", &m_pd3dGeometryShaderBlob));
	else
		return(CompileShaderFromFile(L"Particle.hlsl", "GSParticleDraw", "gs_5_1", &m_pd3dGeometryShaderBlob));
}

D3D12_SHADER_BYTECODE CParticleShader::CreatePixelShader(int nPipelineState)
{
	if (nPipelineState == 0)
		return(CShader::CreatePixelShader(0));
	else
		return(CompileShaderFromFile(L"Particle.hlsl", "PSParticleDraw", "ps_5_1", &m_pd3dPixelShaderBlob));
}

D3D12_INPUT_LAYOUT_DESC CParticleShader::CreateInputLayout()
{
	D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
	UINT nInputElementDescs = 4;
	D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

	pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[1] = { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[2] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	pd3dInputElementDescs[3] = { "PARTICLETYPE", 0, DXGI_FORMAT_R32_UINT, 0, 28, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
	d3dInputLayoutDesc.NumElements = nInputElementDescs;

	return(d3dInputLayoutDesc);
}

D3D12_STREAM_OUTPUT_DESC CParticleShader::CreateStreamOuputState(int nPipelineState)
{
	D3D12_STREAM_OUTPUT_DESC d3dStreamOutputDesc;
	::ZeroMemory(&d3dStreamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));

	if (nPipelineState == 0)
	{
		UINT nStreamOutputDecls = 4;
		D3D12_SO_DECLARATION_ENTRY* pd3dStreamOutputDecls = new D3D12_SO_DECLARATION_ENTRY[nStreamOutputDecls];
		pd3dStreamOutputDecls[0] = { 0, "POSITION", 0, 0, 3, 0 };
		pd3dStreamOutputDecls[1] = { 0, "VELOCITY", 0, 0, 3, 0 };
		pd3dStreamOutputDecls[2] = { 0, "LIFETIME", 0, 0, 1, 0 };
		pd3dStreamOutputDecls[3] = { 0, "PARTICLETYPE", 0, 0, 1, 0 };

		UINT* pBufferStrides = new UINT[1];
		pBufferStrides[0] = sizeof(CParticleVertex);

		d3dStreamOutputDesc.NumEntries = nStreamOutputDecls;
		d3dStreamOutputDesc.pSODeclaration = pd3dStreamOutputDecls;
		d3dStreamOutputDesc.NumStrides = 1;
		d3dStreamOutputDesc.pBufferStrides = pBufferStrides;
		d3dStreamOutputDesc.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;
	}

	return(d3dStreamOutputDesc);
}

D3D12_BLEND_DESC CParticleShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}

D3D12_RASTERIZER_DESC CParticleShader::CreateRasterizerState()
{
	D3D12_RASTERIZER_DESC d3dRasterizerDesc;
	::ZeroMemory(&d3dRasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	d3dRasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	d3dRasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	d3dRasterizerDesc.FrontCounterClockwise = FALSE;
	d3dRasterizerDesc.DepthBias = 0;
	d3dRasterizerDesc.DepthBiasClamp = 0.0f;
	d3dRasterizerDesc.SlopeScaledDepthBias = 0.0f;
	d3dRasterizerDesc.DepthClipEnable = TRUE;
	d3dRasterizerDesc.MultisampleEnable = FALSE;
	d3dRasterizerDesc.AntialiasedLineEnable = FALSE;
	d3dRasterizerDesc.ForcedSampleCount = 0;
	d3dRasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return(d3dRasterizerDesc);
}

D3D12_DEPTH_STENCIL_DESC CParticleShader::CreateDepthStencilState()
{
	//D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	//::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	//d3dDepthStencilDesc.DepthEnable = FALSE;
	//d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	//d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	//d3dDepthStencilDesc.StencilEnable = FALSE;
	//d3dDepthStencilDesc.StencilReadMask = 0x00;
	//d3dDepthStencilDesc.StencilWriteMask = 0x00;
	//d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	//d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	//d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	//return(d3dDepthStencilDesc);
	D3D12_DEPTH_STENCIL_DESC d3dDepthStencilDesc;
	::ZeroMemory(&d3dDepthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	d3dDepthStencilDesc.DepthEnable = TRUE;
	d3dDepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	d3dDepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS; //D3D12_COMPARISON_FUNC_LESS_EQUAL
	d3dDepthStencilDesc.StencilEnable = FALSE;
	d3dDepthStencilDesc.StencilReadMask = 0x00;
	d3dDepthStencilDesc.StencilWriteMask = 0x00;
	d3dDepthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	d3dDepthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	d3dDepthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return(d3dDepthStencilDesc);
}

void CParticleShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	::ZeroMemory(&m_d3dPipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));

	m_d3dPipelineStateDesc.pRootSignature = pd3dGraphicsRootSignature;
	m_d3dPipelineStateDesc.VS = CreateVertexShader(nPipelineState);
	m_d3dPipelineStateDesc.GS = CreateGeometryShader(nPipelineState);
	m_d3dPipelineStateDesc.PS = CreatePixelShader(nPipelineState);
	m_d3dPipelineStateDesc.StreamOutput = CreateStreamOuputState(nPipelineState);
	m_d3dPipelineStateDesc.RasterizerState = CreateRasterizerState();
	m_d3dPipelineStateDesc.BlendState = CreateBlendState();
	m_d3dPipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	m_d3dPipelineStateDesc.InputLayout = CreateInputLayout();
	m_d3dPipelineStateDesc.SampleMask = UINT_MAX;
	m_d3dPipelineStateDesc.PrimitiveTopologyType = GetPrimitiveTopologyType(nPipelineState);
	m_d3dPipelineStateDesc.NumRenderTargets = GetNumRenderTargets(nPipelineState);
	m_d3dPipelineStateDesc.RTVFormats[0] = GetRTVFormat(nPipelineState, 0);
	m_d3dPipelineStateDesc.DSVFormat = GetDSVFormat(nPipelineState);
	m_d3dPipelineStateDesc.SampleDesc.Count = 1;
	m_d3dPipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	HRESULT hResult = pd3dDevice->CreateGraphicsPipelineState(&m_d3dPipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)&m_ppd3dPipelineStates[nPipelineState]);
}

void CParticleShader::CreateParticleShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
{
	m_nPipelineStates = 2;
	m_ppd3dPipelineStates = new ID3D12PipelineState * [m_nPipelineStates];

	CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, 0); //Stream Output Pipeline State
	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();
	if (m_pd3dGeometryShaderBlob) m_pd3dGeometryShaderBlob->Release();
	CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, 1); //Draw Pipeline State

	if (m_pd3dVertexShaderBlob) m_pd3dVertexShaderBlob->Release();
	if (m_pd3dPixelShaderBlob) m_pd3dPixelShaderBlob->Release();
	if (m_pd3dGeometryShaderBlob) m_pd3dGeometryShaderBlob->Release();

	if (m_d3dPipelineStateDesc.InputLayout.pInputElementDescs) delete[] m_d3dPipelineStateDesc.InputLayout.pInputElementDescs;
}

///////////////////////////////////////////////////////////////////////////////////////////////

CBlendSkillShader::CBlendSkillShader()
{
}

CBlendSkillShader::~CBlendSkillShader()
{
}

D3D12_BLEND_DESC CBlendSkillShader::CreateBlendState()
{
	D3D12_BLEND_DESC d3dBlendDesc;
	::ZeroMemory(&d3dBlendDesc, sizeof(D3D12_BLEND_DESC));
	d3dBlendDesc.AlphaToCoverageEnable = FALSE;
	d3dBlendDesc.IndependentBlendEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	d3dBlendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	d3dBlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	d3dBlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ONE;
	d3dBlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	d3dBlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	d3dBlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	d3dBlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return(d3dBlendDesc);
}