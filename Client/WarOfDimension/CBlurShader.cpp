#include "stdafx.h"
#include "CBlurShader.h"
#include "Scene.h"
#include "NetworkManager.h"
#include "SceneManager.h"

CBlurShader::CBlurShader()
{
}

CBlurShader::~CBlurShader()
{
	if (m_pTexture) delete m_pTexture;
}

void CBlurShader::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext)
{
	m_pTexture = (CTexture*)pContext;
	if (m_pTexture) {
		CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, m_pTexture->GetTextures());
		CreateShaderResourceViews(pd3dDevice, m_pTexture, 0, 3);
	}
}

void CBlurShader::CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState)
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

D3D12_SHADER_BYTECODE CBlurShader::CreateVertexShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"DeferredRender.hlsl", "VSScreenRectSamplingTextured", "vs_5_1", &m_pd3dVertexShaderBlob));
}

D3D12_SHADER_BYTECODE CBlurShader::CreatePixelShader(int nPipelineState)
{
	return(CompileShaderFromFile(L"Blur.hlsl", "PSBlur", "ps_5_1", &m_pd3dPixelShaderBlob));
}

void CBlurShader::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState)
{
	CShader::Render(pd3dCommandList, pCamera); 
	if (m_pTexture) m_pTexture->UpdateShaderVariables(pd3dCommandList); 

	pd3dCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST); 
	pd3dCommandList->DrawInstanced(6, 1, 0, 0); 
}
