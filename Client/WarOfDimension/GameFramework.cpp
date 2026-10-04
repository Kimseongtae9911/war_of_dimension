//-----------------------------------------------------------------------------
// File: CGameFramework.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "GameFramework.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Frustum.h"
#include "RenderManager.h"
#include "SoundManager.h"

#define TEST_JOB 2	// 0 archer, 1 fighter, 2 swordman, 3 wizzard, 4 ogre, 5 programmer
#define TEST_SKILL1 92
#define TEST_SKILL2 50
#define TEST_SKILL3 51
#define TEST_SKILL4 81

#define BOSS_TEST_SKILL1 112 - 96 + (BOSS_SKILL / 2) + 1
#define BOSS_TEST_SKILL2 113 - 96 + (BOSS_SKILL / 2) + 1
#define BOSS_TEST_SKILL3 114 - 96 + (BOSS_SKILL / 2) + 1
#define BOSS_TEST_SKILL4 115 - 96 + (BOSS_SKILL / 2) + 1

CGameFramework::CGameFramework()
{
	m_pdxgiFactory = NULL;
	m_pdxgiSwapChain = NULL;
	m_pd3dDevice = NULL;

	for (int i = 0; i < m_nSwapChainBuffers; i++) m_ppd3dSwapChainBackBuffers[i] = NULL;
	m_nSwapChainBufferIndex = 0;

	m_pd3dCommandAllocator = NULL;
	m_pd3dCommandQueue = NULL;
	m_pd3dCommandList = NULL;
	m_pd3dMTCommandList = NULL;
	m_pd3dMTCommandAllocator = NULL;

	m_pd3dRtvDescriptorHeap = NULL;
	m_pd3dDsvDescriptorHeap = NULL;

	m_pPipelineState = NULL;

	m_hFenceEvent = NULL;
	m_pd3dFence = NULL;
	for (int i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_nWndClientWidth = FRAME_BUFFER_WIDTH;
	m_nWndClientHeight = FRAME_BUFFER_HEIGHT;

	m_pScene = NULL;
	m_pPlayer = NULL;

	_tcscpy_s(m_pszFrameRate, _T("WOD ("));

	m_hInstance = nullptr;
	m_hWnd = nullptr;
	m_ptOldCursorPos = {};
}

CGameFramework::~CGameFramework()
{
}

bool CGameFramework::OnCreate(HINSTANCE hInstance, HWND hMainWnd)
{
	m_hInstance = hInstance;
	m_hWnd = hMainWnd;
	 
#ifdef Test
	cout << "Initializing Client" << endl;
#endif
	CreateDirect3DDevice();
	CreateCommandQueueAndList();

	CreateRtvAndDsvDescriptorHeaps();
	CreateSwapChain();
	CreateDepthStencilView();

	HRESULT res = CoInitialize(NULL);

	CreateShadowMapCamera();
	BuildObjects();
	CreateShadowMap();

#ifdef Test
	cout << "Initialize Finish" << endl;
#endif
	return(true);
}

void CGameFramework::CreateSwapChain()
{
	RECT rcClient;
	::GetClientRect(m_hWnd, &rcClient);
	m_nWndClientWidth = rcClient.right - rcClient.left;
	m_nWndClientHeight = rcClient.bottom - rcClient.top;

#ifdef _WITH_CREATE_SWAPCHAIN_FOR_HWND
	DXGI_SWAP_CHAIN_DESC1 dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC1));
	dxgiSwapChainDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.Height = m_nWndClientHeight;
	dxgiSwapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.Scaling = DXGI_SCALING_NONE;
	//dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	//수직 동기화
	dxgiSwapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC dxgiSwapChainFullScreenDesc;
	::ZeroMemory(&dxgiSwapChainFullScreenDesc, sizeof(DXGI_SWAP_CHAIN_FULLSCREEN_DESC));
	dxgiSwapChainFullScreenDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainFullScreenDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainFullScreenDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiSwapChainFullScreenDesc.Windowed = TRUE;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChainForHwnd(m_pd3dCommandQueue, m_hWnd, &dxgiSwapChainDesc, &dxgiSwapChainFullScreenDesc, NULL, (IDXGISwapChain1 **)&m_pdxgiSwapChain);
#else
	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = m_nSwapChainBuffers;
	dxgiSwapChainDesc.BufferDesc.Width = m_nWndClientWidth;
	dxgiSwapChainDesc.BufferDesc.Height = m_nWndClientHeight;
	//dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;	//수직 동기화
	dxgiSwapChainDesc.OutputWindow = m_hWnd;
	dxgiSwapChainDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;

	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = m_pdxgiFactory->CreateSwapChain(m_pd3dCommandQueue, &dxgiSwapChainDesc, (IDXGISwapChain **)&m_pdxgiSwapChain);
#endif
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = m_pdxgiFactory->MakeWindowAssociation(m_hWnd, DXGI_MWA_NO_ALT_ENTER);

#ifndef _WITH_SWAPCHAIN_FULLSCREEN_STATE
	CreateRenderTargetViews();
#endif
}

void CGameFramework::CreateDirect3DDevice()
{
	HRESULT hResult;

	UINT nDXGIFactoryFlags = 0;
#if defined(_DEBUG)
	ID3D12Debug *pd3dDebugController = NULL;
	hResult = D3D12GetDebugInterface(__uuidof(ID3D12Debug), (void **)&pd3dDebugController);
	if (pd3dDebugController)
	{
		pd3dDebugController->EnableDebugLayer();
		pd3dDebugController->Release();
	}
	nDXGIFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	hResult = ::CreateDXGIFactory2(nDXGIFactoryFlags, __uuidof(IDXGIFactory4), (void **)&m_pdxgiFactory);

	IDXGIAdapter1 *pd3dAdapter = NULL;

	for (UINT i = 0; DXGI_ERROR_NOT_FOUND != m_pdxgiFactory->EnumAdapters1(i, &pd3dAdapter); i++)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pd3dAdapter->GetDesc1(&dxgiAdapterDesc);
		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;
		if (SUCCEEDED(D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice))) break;
	}

	if (!pd3dAdapter)
	{
		m_pdxgiFactory->EnumWarpAdapter(_uuidof(IDXGIFactory4), (void **)&pd3dAdapter);
		hResult = D3D12CreateDevice(pd3dAdapter, D3D_FEATURE_LEVEL_12_0, _uuidof(ID3D12Device), (void **)&m_pd3dDevice);
	}

	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS d3dMsaaQualityLevels;
	d3dMsaaQualityLevels.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dMsaaQualityLevels.SampleCount = 4;
	d3dMsaaQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	d3dMsaaQualityLevels.NumQualityLevels = 0;
	hResult = m_pd3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &d3dMsaaQualityLevels, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	m_nMsaa4xQualityLevels = d3dMsaaQualityLevels.NumQualityLevels;
	m_bMsaa4xEnable = (m_nMsaa4xQualityLevels > 1) ? true : false;

	hResult = m_pd3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void **)&m_pd3dFence);
	for (UINT i = 0; i < m_nSwapChainBuffers; i++) m_nFenceValues[i] = 0;

	m_hFenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);

	::gnCbvSrvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

	if (pd3dAdapter) pd3dAdapter->Release();
}

void CGameFramework::CreateCommandQueueAndList()
{
	HRESULT hResult;

	D3D12_COMMAND_QUEUE_DESC d3dCommandQueueDesc;
	::ZeroMemory(&d3dCommandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	d3dCommandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	d3dCommandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = m_pd3dDevice->CreateCommandQueue(&d3dCommandQueueDesc, _uuidof(ID3D12CommandQueue), (void **)&m_pd3dCommandQueue);

	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dCommandAllocator);
	hResult = m_pd3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void **)&m_pd3dMTCommandAllocator);

	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dCommandList);
	hResult = m_pd3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pd3dMTCommandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&m_pd3dMTCommandList);
	hResult = m_pd3dCommandList->Close();
	hResult = m_pd3dMTCommandList->Close();
}

void CGameFramework::CreateRtvAndDsvDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	::ZeroMemory(&d3dDescriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	d3dDescriptorHeapDesc.NumDescriptors = m_nSwapChainBuffers + DEFERREDNUM + 1;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dRtvDescriptorHeap);
	::gnRtvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	d3dDescriptorHeapDesc.NumDescriptors = 2;
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = m_pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dDsvDescriptorHeap);
	::gnDsvDescriptorIncrementSize = m_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
}

//void CGameFramework::CreateRenderTargetViews()
//{
//	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
//	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
//	{
//		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void **)&m_ppd3dSwapChainBackBuffers[i]);
//		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], NULL, d3dRtvCPUDescriptorHandle);
//		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
//	}
//}

void CGameFramework::CreateRenderTargetViews()
{
	D3D12_RENDER_TARGET_VIEW_DESC d3dRenderTargetViewDesc;
	d3dRenderTargetViewDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	d3dRenderTargetViewDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
	d3dRenderTargetViewDesc.Texture2D.MipSlice = 0;
	d3dRenderTargetViewDesc.Texture2D.PlaneSlice = 0;

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT i = 0; i < m_nSwapChainBuffers; i++)
	{
		m_pdxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&m_ppd3dSwapChainBackBuffers[i]);
		m_pd3dDevice->CreateRenderTargetView(m_ppd3dSwapChainBackBuffers[i], &d3dRenderTargetViewDesc, d3dRtvCPUDescriptorHandle);
		m_pd3dSwapChainBackBufferRTVCPUHandles[i] = d3dRtvCPUDescriptorHandle;
		d3dRtvCPUDescriptorHandle.ptr += ::gnRtvDescriptorIncrementSize;
	}
}

void CGameFramework::CreateDepthStencilView()
{
	D3D12_RESOURCE_DESC d3dResourceDesc;
	d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3dResourceDesc.Alignment = 0;
	d3dResourceDesc.Width = m_nWndClientWidth;
	d3dResourceDesc.Height = m_nWndClientHeight;
	d3dResourceDesc.DepthOrArraySize = 1;
	d3dResourceDesc.MipLevels = 1;
	d3dResourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dResourceDesc.SampleDesc.Count = (m_bMsaa4xEnable) ? 4 : 1;
	d3dResourceDesc.SampleDesc.Quality = (m_bMsaa4xEnable) ? (m_nMsaa4xQualityLevels - 1) : 0;
	d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES d3dHeapProperties;
	::ZeroMemory(&d3dHeapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	d3dHeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapProperties.CreationNodeMask = 1;
	d3dHeapProperties.VisibleNodeMask = 1;

	D3D12_CLEAR_VALUE d3dClearValue;
	d3dClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	d3dClearValue.DepthStencil.Depth = 1.0f;
	d3dClearValue.DepthStencil.Stencil = 0;

	m_pd3dDevice->CreateCommittedResource(&d3dHeapProperties, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &d3dClearValue, __uuidof(ID3D12Resource), (void **)&m_pd3dDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC d3dDepthStencilViewDesc;
	::ZeroMemory(&d3dDepthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	d3dDepthStencilViewDesc.Format = DXGI_FORMAT_D32_FLOAT;
	d3dDepthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	d3dDepthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescriptorCPUHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_pd3dDevice->CreateDepthStencilView(m_pd3dDepthStencilBuffer, &d3dDepthStencilViewDesc, m_DSVDescriptorCPUHandle);
}

void CGameFramework::CreateShadowMap()
{
	m_ShadowMap = make_unique<ShadowMap>(m_pd3dDevice, 8192, 8192);
	//m_ShadowMap = new ShadowMap(m_pd3dDevice, 4096, 4096);
	D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dDsvCPUDescriptorHandle.ptr += (::gnDsvDescriptorIncrementSize);

	/// 

	m_ShadowMap->BuildDescriptors(m_pPostProcessingShader->GetCPUSrvDescriptorNextHandle(), 
		m_pPostProcessingShader->GetGPUSrvDescriptorNextHandle(), d3dDsvCPUDescriptorHandle);

	{
		D3D12_GRAPHICS_PIPELINE_STATE_DESC PsoDesc{};

		UINT nInputElementDescs = 1;
		D3D12_INPUT_ELEMENT_DESC* pd3dInputElementDescs = new D3D12_INPUT_ELEMENT_DESC[nInputElementDescs];

		pd3dInputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

		D3D12_INPUT_LAYOUT_DESC d3dInputLayoutDesc;
		d3dInputLayoutDesc.pInputElementDescs = pd3dInputElementDescs;
		d3dInputLayoutDesc.NumElements = nInputElementDescs;

		PsoDesc.InputLayout = d3dInputLayoutDesc;

		PsoDesc.pRootSignature = m_pScene->GetRootSignature();
		ID3DBlob* ppd3dShaderBlob = NULL;
		ID3DBlob* pd3dPixelShaderBlob = NULL;
		PsoDesc.VS = CompileShaderFromFile(L"Shadowmap.hlsl", "VSShadowMap", "vs_5_1", &ppd3dShaderBlob);
		PsoDesc.PS = CompileShaderFromFile(L"Shadowmap.hlsl", "PSShadowMap", "ps_5_1", &pd3dPixelShaderBlob);
		{
			PsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
			PsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
			PsoDesc.RasterizerState.FrontCounterClockwise = FALSE;
			PsoDesc.RasterizerState.DepthBias = 100000;
			PsoDesc.RasterizerState.DepthBiasClamp = 0.0f;
			PsoDesc.RasterizerState.SlopeScaledDepthBias = 1.0f;
			PsoDesc.RasterizerState.DepthClipEnable = TRUE;
			PsoDesc.RasterizerState.MultisampleEnable = FALSE;
			PsoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
			PsoDesc.RasterizerState.ForcedSampleCount = 0;
			PsoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

			PsoDesc.BlendState.AlphaToCoverageEnable = FALSE;
			PsoDesc.BlendState.IndependentBlendEnable = FALSE;
			PsoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
			PsoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
			PsoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
			PsoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
			PsoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
			PsoDesc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
			PsoDesc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
			PsoDesc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
			PsoDesc.BlendState.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
			PsoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

			PsoDesc.DepthStencilState.DepthEnable = TRUE;
			PsoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
			PsoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
			PsoDesc.DepthStencilState.StencilEnable = FALSE;
			PsoDesc.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
			PsoDesc.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
			PsoDesc.DepthStencilState.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
			PsoDesc.DepthStencilState.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
			PsoDesc.DepthStencilState.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		}

		PsoDesc.SampleMask = UINT_MAX;
		PsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		PsoDesc.NumRenderTargets = 0;
		PsoDesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
		PsoDesc.SampleDesc.Count = 1;
		PsoDesc.SampleDesc.Quality = 0;
		PsoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

		HRESULT hResult = m_pd3dDevice->CreateGraphicsPipelineState(&PsoDesc, __uuidof(ID3D12PipelineState), (void**)&m_pPipelineState);

		if (ppd3dShaderBlob) ppd3dShaderBlob->Release();
		if (pd3dPixelShaderBlob) pd3dPixelShaderBlob->Release();

		if (PsoDesc.InputLayout.pInputElementDescs) delete[] PsoDesc.InputLayout.pInputElementDescs;
	}
}

void CGameFramework::CreateShadowMapCamera()
{
	UINT ncbElementBytes = ((sizeof(VS_CB_CAMERA_INFO) + 255) & ~255); //256의 배수
	m_pShadowCamera = ::CreateBufferResource(m_pd3dDevice, m_pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	m_pShadowCamera->Map(0, NULL, (void**)&m_pShadowMappedCamera);
}

void CGameFramework::ChangeSwapChainState()
{
	WaitForGpuComplete();

	BOOL bFullScreenState = FALSE;
	m_pdxgiSwapChain->GetFullscreenState(&bFullScreenState, NULL);
	m_pdxgiSwapChain->SetFullscreenState(~bFullScreenState, NULL);

	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = m_nWndClientWidth;
	dxgiTargetParameters.Height = m_nWndClientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	m_pdxgiSwapChain->ResizeTarget(&dxgiTargetParameters);

	if (m_pBlurBuffer) m_pBlurBuffer->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	m_pdxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	m_pdxgiSwapChain->ResizeBuffers(m_nSwapChainBuffers, m_nWndClientWidth, m_nWndClientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);

	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	CreateRenderTargetViews();
}

void CGameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) m_pScene->OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);

	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	}
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&m_ptOldCursorPos);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		POINT clickPos = m_ptOldCursorPos;
		break;
	}
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

void CGameFramework::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	if (m_pScene) {
		m_pScene->OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
	}

	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			if (CheckUIPopUp()) {

			}
			else {
				::PostQuitMessage(0);
			}
			break;
		case VK_INSERT:
			NetworkManager::GetInstance()->lerpPercentage += 0.01f;
			break;
		case VK_DELETE:
			NetworkManager::GetInstance()->lerpPercentage -= 0.01f;
			break;
		case VK_RETURN:
			//m_pPlayer->SetAddDissolveState(m_GameTimer.GetTimeElapsed());
			//bOneTime = !bOneTime;
			break;
		case VK_F6:
		{
			bDebugRendering = !bDebugRendering;
		}
		break;		
		case VK_F7:
			m_pPlayer->ModifyModel();
			NetworkManager::GetInstance()->SendModelCustomizePacket(m_pPlayer->GetCustomizeInfo());
			break;
		case VK_F9:
			isBlurRender = !isBlurRender;
			//ChangeSwapChainState();
			break;
		case '4': {
			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::TITLE || SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
				break;

			for (int i = 0; i < NetworkManager::GetInstance()->readySceneInfo->selectSkills.size(); ++i) {
				for (int j = 0; j < NetworkManager::GetInstance()->readySceneInfo->selectSkills[i].size(); ++j) {
					if (i == 3) {
						NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j] = BOSS_SKILL / 2;

					}
					else {
						NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j] = PLAYER_SKILL / 4;
					}
				}
			}
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[0][0] = TEST_SKILL1;//Right
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[0][1] = TEST_SKILL2;//shift
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[0][2] = TEST_SKILL3;//q
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[0][3] = TEST_SKILL4;//r

			NetworkManager::GetInstance()->readySceneInfo->playerJobs[1] = 1;
			NetworkManager::GetInstance()->readySceneInfo->playerJobs[2] = 2;
			NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] = 4;

			//test job change
			NetworkManager::GetInstance()->readySceneInfo->playerJobs[0] = TEST_JOB;

			SceneManager::GetInstance()->m_nCurScene = SCENEKIND::READY;
			testing = true;
			CS_TEST_CHANGE_SERVER_PACKET* p = new CS_TEST_CHANGE_SERVER_PACKET;
			p->size = sizeof(CS_TEST_CHANGE_SERVER_PACKET);
			p->type = CS_TEST_CHANGE_SERVER;
			p->boss = false;
			NetworkManager::GetInstance()->SendPacket(p);
			m_nCurScene = SCENEKIND::INGAME;
			break;
		}
		case '5':
			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::TITLE || SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
				break;

			NetworkManager::GetInstance()->TestReady();	//Boss Test
			testing = true;
			break;
		case '6':
			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::TITLE || SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
				break;

			NetworkManager::GetInstance()->TestReady(true);	//Hero Test
			testing = true;
			break;
		case '7':
		{
			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::TITLE || SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
				break;

			for (int i = 0; i < NetworkManager::GetInstance()->readySceneInfo->selectSkills.size(); ++i) {
				for (int j = 0; j < NetworkManager::GetInstance()->readySceneInfo->selectSkills[i].size(); ++j) {
					if (i == 3) {
						NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j] = BOSS_SKILL / 2;
					}
					else {
						NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j] = PLAYER_SKILL / 4;
					}
				}
			}

			NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][0] = BOSS_TEST_SKILL1;//Right
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][1] = BOSS_TEST_SKILL2;//shift
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][2] = BOSS_TEST_SKILL3;//q
			NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][3] = BOSS_TEST_SKILL4;//r

			//test job change
			NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] = TEST_JOB;

			//CTextureShader::GetInstance()->SetBossJob(BOSSJOB::PROGRAMMER);
			SceneManager::GetInstance()->SetOrder(ORDER::BOSS);
			SceneManager::GetInstance()->m_nCurScene = SCENEKIND::READY;
			testing = true;
			CS_TEST_CHANGE_SERVER_PACKET* p = new CS_TEST_CHANGE_SERVER_PACKET;
			p->size = sizeof(CS_TEST_CHANGE_SERVER_PACKET);
			p->type = CS_TEST_CHANGE_SERVER;
			p->boss = true;
			NetworkManager::GetInstance()->SendPacket(p);
			break;
		}
		case 'C':
			Frustum::GetInstance()->Print();
			Frustum::GetInstance()->m_bToggle = !Frustum::GetInstance()->m_bToggle;
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}
}

LRESULT CALLBACK CGameFramework::OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
		case WM_ACTIVATE:
		{
			if (LOWORD(wParam) == WA_INACTIVE) {
				m_GameTimer.Stop();
			}
			else
				m_GameTimer.Start();
			break;
		}
		case WM_SIZE:
			break;
		case WM_LBUTTONDOWN:
        case WM_RBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        case WM_MOUSEMOVE:
			OnProcessingMouseMessage(hWnd, nMessageID, wParam, lParam);
            break;
		case WM_IME_COMPOSITION:
        case WM_KEYDOWN:
        case WM_KEYUP:
		case WM_CHAR:
			OnProcessingKeyboardMessage(hWnd, nMessageID, wParam, lParam);
			break;
	}
	return(0);
}

void CGameFramework::ChangeScene(SCENEKIND nSceneKind)
{
	if (nSceneKind != SceneManager::GetInstance()->m_nCurScene)
	{
		HANDLE hThread;
		if (nSceneKind != SCENEKIND::TITLE)
		{
			hThread = CreateThread(NULL, 0, ThreadProc, (void*)this, 0, NULL);

			if (hThread == NULL)
			{
				cout << "Thread fail" << endl;
			}
		}

		ChangeSceneReleaseObject();
		SceneManager::GetInstance()->m_nCurScene = nSceneKind;
		SoundManager::GetInstance()->Stop_All();

		switch (nSceneKind)
		{
		case SCENEKIND::TITLE:
		{
			NetworkManager::GetInstance()->playerScene = nSceneKind;
			::ReleaseCapture();

			m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

			m_pScene = new CTitleScene();
			if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

			CTitlePlayer* pPlayer = new CTitlePlayer(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);

			m_pScene->m_pPlayer = m_pPlayer = pPlayer;
			m_pCamera = m_pPlayer->GetCamera();

			m_pTextureShader->SetScene(SceneManager::GetInstance()->m_nCurScene);
			m_pTextureShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);

			Frustum::GetInstance()->m_xmfCamera4x4View = m_pCamera->GetViewMatrix();
			Frustum::GetInstance()->m_xmf4x4CameraProjection = m_pCamera->GetProjectionMatrix();
			Frustum::GetInstance()->Update();
			break;
		}
		case SCENEKIND::LOBBY:
		{
			::ReleaseCapture();

			if (S_OK != m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL)) {
				cout << "CommandList Reset Fail" << endl;
			}

			m_pScene = new CLobbyScene();
			if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

			CLoadedModelInfo* pClient = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", NULL);
			m_pScene->BuildOtherClient(m_pd3dDevice, m_pd3dCommandList, pClient);

			CGamePlayer* pPlayer = new CGamePlayer(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, pClient, NULL);
			pPlayer->SetObjectID(1);
			NetworkManager::GetInstance()->myClient = m_pScene->m_pPlayer = m_pPlayer = pPlayer;
			m_pCamera = m_pPlayer->GetCamera();
			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::CHARACTER];
			if (pClient)
				delete pClient;

			//Create Once
			if (!m_pPostProcessingShader) {
				m_pPostProcessingShader = new CPostProcessingShader();
				m_pPostProcessingShader->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
				m_pPostProcessingShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);

				D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
				d3dRtvCPUDescriptorHandle.ptr += (::gnRtvDescriptorIncrementSize * m_nSwapChainBuffers);

				DXGI_FORMAT pdxgiResourceFormats[DEFERREDNUM] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };
				m_pPostProcessingShader->CreateResourcesAndViews(m_pd3dDevice, m_pd3dCommandList, DEFERREDNUM, pdxgiResourceFormats, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, d3dRtvCPUDescriptorHandle, DEFERREDNUM + 1); //SRV to (Render Targets) + (Depth Buffer)

				DXGI_FORMAT pdxgiDepthSrvFormats[1] = { DXGI_FORMAT_R32_FLOAT };
				m_pPostProcessingShader->CreateShaderResourceViews(m_pd3dDevice, 1, &m_pd3dDepthStencilBuffer, pdxgiDepthSrvFormats);
			}
			m_pTextureShader->SetScene(SceneManager::GetInstance()->m_nCurScene);
			m_pTextureShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);

			m_pBlendShader = new CBlendObjectShader();
			m_pBlendShader->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
			m_pBlendShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);

#ifndef WITH_DATABASE
			NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name);
#endif // !WITH_DATABASE

			if (m_isExecutedOnce) {
				NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name, NetworkManager::GetInstance()->clientPassword);
			}
			else {
				m_isExecutedOnce = true;
			}

			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::SHADER];
			
			NetworkManager::GetInstance()->SendPortNumPacket();

			break;
		}
		case SCENEKIND::READY:
		{
			::ReleaseCapture();


			if (S_OK != m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL)) {
				cout << "CommandList Reset Fail" << endl;
			}


			m_pScene = new CReadyScene();
			if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
			

			
			CLoadedModelInfo* pClient = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", NULL);

			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::CHARACTER];
			m_pScene->BuildOtherClient(m_pd3dDevice, m_pd3dCommandList, pClient);
			SceneManager::GetInstance()->SetOrder(static_cast<ORDER>(NetworkManager::GetInstance()->GetId()));
			CReadyPlayer* pPlayer = new CReadyPlayer(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, pClient);

			if (pClient)
				delete pClient;
			
			pPlayer->SetObjectID(1);
			NetworkManager::GetInstance()->myClient = m_pScene->m_pPlayer = m_pPlayer = pPlayer;
			m_pCamera = m_pPlayer->GetCamera();
			dynamic_cast<CGamePlayer*>(m_pPlayer)->SetReadyAnim(static_cast<int>(JOB::ARCHER));

			m_pTextureShader->SetScene(SceneManager::GetInstance()->m_nCurScene);
			m_pTextureShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);

			//Register to Game Server
			NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name);
			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::SHADER];

			break;
		}
		case SCENEKIND::INGAME:
		{
			if (S_OK != m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL)) {
				cout << "CommandList Reset Fail" << endl;
			}

			m_pScene = new CIngameScene();
			if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

			CLoadedModelInfo* pClient = CGameObject::LoadGeometryAndAnimationFromFile(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ModularModel.bin", NULL);

			m_pScene->BuildOtherClient(m_pd3dDevice, m_pd3dCommandList, pClient);

			// 인게임 테스트 패킷
			if (testing) {
				NetworkManager::GetInstance()->SendTestIngamePacket(SceneManager::GetInstance()->m_Name);
			}

			// 인게임 테스트 패킷 보다 먼저 수행해야 networkManager의 myClient값이 null이 아님
			CGamePlayer* pPlayer = new CGamePlayer(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, pClient);
			if (pClient)
				delete pClient;

			pPlayer->SetObjectID(1);
			NetworkManager::GetInstance()->myClient = m_pScene->m_pPlayer = m_pPlayer = pPlayer;
			m_pCamera = m_pPlayer->GetCamera();
			NetworkManager::GetInstance()->playerScene = SCENEKIND::INGAME;

			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::CHARACTER];

			m_pTextureShader->SetScene(SceneManager::GetInstance()->m_nCurScene);
			m_pTextureShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);

			if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
			{
				m_pTextureShader->SetPlayerSkillServerIngame(NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()]);
			}
			else
				m_pTextureShader->SetBossSkillServerIngame(NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()]);

			m_pDissolveTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
			m_pDissolveTexture->LoadTextureFromDDSFile(m_pd3dDevice, m_pd3dCommandList, L"Model/Textures/dissolve.dds", RESOURCE_TEXTURE2D, 0);
			m_pScene->CreateShaderResourceViews(m_pd3dDevice, m_pDissolveTexture, 0, 20);

			SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::SHADER];

			if (testing) {
				NetworkManager::GetInstance()->SendTestIngamePacket();
			}

			break;
		}
			
		}
		
		if (S_OK != m_pd3dCommandList->Close()) {
			cout << "CommandList Close Fail" << endl;
		}
		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

		WaitForGpuComplete();

		if (m_pScene) m_pScene->ReleaseUploadBuffers();
		if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

		if (nSceneKind != SCENEKIND::TITLE)
		{
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
		}

		m_GameTimer.Reset();	
	}
}

void CGameFramework::Update()
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY)//Show Ready Texture
	{
		if (m_pUILayer && m_pTextureShader)
		{
			m_pTextureShader->SetReadySceneSkillUI(m_pUILayer->m_bSkillPopUpClick);
			if (NetworkManager::GetInstance()->playerScene == SCENEKIND::READY)
			{
				for (int i = 0; i < INGAME_PLAYER; ++i)
				{
					if (NetworkManager::GetInstance()->GetId() == i);
					else
					{
						m_pUILayer->m_bReadyTextShow[i] = NetworkManager::GetInstance()->readySceneInfo->playerReadys[i];
						if (i < 3)
						{
							m_pTextureShader->SetPlayerJOB(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[i]), static_cast<ORDER>(i));
							reinterpret_cast<CPlayerObject*>(m_pScene->m_ppOtherClient[i])->SetWeapon(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[i]));
							m_pTextureShader->SetSkillServer(NetworkManager::GetInstance()->readySceneInfo->selectSkills[i], static_cast<ORDER>(i));
							for (int j = 0; j < 4; ++j)
							{							
								if (NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j] != 0)
								{
									m_pUILayer->m_bReadyPlayerSkillRects[i][j] = false;
								}
								else
								{
									m_pUILayer->m_bReadyPlayerSkillRects[i][j] = true;									
								}
							}
							reinterpret_cast<COtherClientPlayer*>(m_pScene->m_ppOtherClient[i])->SetReadyAnim(NetworkManager::GetInstance()->readySceneInfo->playerJobs[i]);
						}
					}
				}
			}
		}
	}
}

void CGameFramework::CreateShaderVariables()
{
	UINT ncbElementBytes = ((sizeof(UINT) + 255) & ~255); //256의 배수
	m_pd3dcbTime = ::CreateBufferResource(m_pd3dDevice, m_pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbTime->Map(0, NULL, (void**)&m_pTime);
}

void CGameFramework::UpdateShaderVariables()
{
	float fCurrentTime = m_GameTimer.GetTotalTime();
	float fElapsedTime = m_GameTimer.GetTimeElapsed();

	//m_pd3dCommandList->SetGraphicsRoot32BitConstants(20, 1, &fCurrentTime, 0);
	//m_pd3dCommandList->SetGraphicsRoot32BitConstants(20, 1, &fElapsedTime, 1);

	TIME temp;
	temp.fCurrentTime = fCurrentTime;
	temp.fElapsedTime = fElapsedTime;

	::memcpy(m_pTime, &temp, sizeof(TIME));

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbTime->GetGPUVirtualAddress();
	m_pd3dCommandList->SetGraphicsRootConstantBufferView(18, d3dcbLightsGpuVirtualAddress);

	if (m_pDissolveTexture)
	{
		m_pDissolveTexture->UpdateShaderVariables(m_pd3dCommandList);
	}
}

void CGameFramework::CreateGraphicsRootSignature()
{
	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[17];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 6; //t6: gtxtAlbedoTexture
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 7; //t7: gtxtSpecularTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 8; //t8: gtxtNormalTexture
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 9; //t9: gtxtMetallicTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 10; //t10: gtxtEmissionTexture
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 11; //t11: gtxtEmissionTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 12; //t12: gtxtEmissionTexture
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[7].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[7].NumDescriptors = 1;
	pd3dDescriptorRanges[7].BaseShaderRegister = 13; //t13: gtxtSkyBoxTexture
	pd3dDescriptorRanges[7].RegisterSpace = 0;
	pd3dDescriptorRanges[7].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[8].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[8].NumDescriptors = 1;
	pd3dDescriptorRanges[8].BaseShaderRegister = 1; //t1: gtxtTerrainBaseTexture
	pd3dDescriptorRanges[8].RegisterSpace = 0;
	pd3dDescriptorRanges[8].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[9].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[9].NumDescriptors = 1;
	pd3dDescriptorRanges[9].BaseShaderRegister = 2; //t2: gtxtTerrainDetailTexture
	pd3dDescriptorRanges[9].RegisterSpace = 0;
	pd3dDescriptorRanges[9].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[10].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[10].NumDescriptors = 4;
	pd3dDescriptorRanges[10].BaseShaderRegister = 14; //t14~17: bilboard
	pd3dDescriptorRanges[10].RegisterSpace = 0;
	pd3dDescriptorRanges[10].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[11].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[11].NumDescriptors = DEFERREDNUM + 1;
	pd3dDescriptorRanges[11].BaseShaderRegister = 18; //t18: DEFERREDNUM is 6
	pd3dDescriptorRanges[11].RegisterSpace = 0;
	pd3dDescriptorRanges[11].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[12].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[12].NumDescriptors = 1;
	pd3dDescriptorRanges[12].BaseShaderRegister = 25; //Depth Buffer
	pd3dDescriptorRanges[12].RegisterSpace = 0;
	pd3dDescriptorRanges[12].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[13].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[13].NumDescriptors = 1;
	pd3dDescriptorRanges[13].BaseShaderRegister = 26; //if MAX_DEPTH_TEXTURES == 1  t26 dissolve Texture 
	pd3dDescriptorRanges[13].RegisterSpace = 0;
	pd3dDescriptorRanges[13].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[14].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[14].NumDescriptors = 1;
	pd3dDescriptorRanges[14].BaseShaderRegister = 27; //t27: gtxtParticleTexture
	pd3dDescriptorRanges[14].RegisterSpace = 0;
	pd3dDescriptorRanges[14].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[15].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[15].NumDescriptors = 1;
	pd3dDescriptorRanges[15].BaseShaderRegister = 28; //t28: gtxtRandomTexture
	pd3dDescriptorRanges[15].RegisterSpace = 0;
	pd3dDescriptorRanges[15].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[16].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[16].NumDescriptors = 1;
	pd3dDescriptorRanges[16].BaseShaderRegister = 29; //t29: gtxtRandomOnSphereTexture
	pd3dDescriptorRanges[16].RegisterSpace = 0;
	pd3dDescriptorRanges[16].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER pd3dRootParameters[26];

	pd3dRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[0].Descriptor.ShaderRegister = 1; //Camera
	pd3dRootParameters[0].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	//pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	//pd3dRootParameters[1].Constants.Num32BitValues = 35;
	//pd3dRootParameters[1].Constants.ShaderRegister = 2; //GameObject
	//pd3dRootParameters[1].Constants.RegisterSpace = 0;
	//pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[1].Descriptor.ShaderRegister = 2; //GameObject
	pd3dRootParameters[1].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[2].Descriptor.ShaderRegister = 4; //Lights
	pd3dRootParameters[2].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[3].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[3].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pd3dRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pd3dRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pd3dRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[6].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pd3dRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[7].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[4]);
	pd3dRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[5]);
	pd3dRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[9].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[6]);
	pd3dRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[10].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[7]);
	pd3dRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[11].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[11].Descriptor.ShaderRegister = 7; //Skinned Bone Offsets
	pd3dRootParameters[11].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[11].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[12].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[12].Descriptor.ShaderRegister = 8; //Skinned Bone Transforms
	pd3dRootParameters[12].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[12].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	pd3dRootParameters[13].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[13].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[13].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[10]);
	pd3dRootParameters[13].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[14].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[14].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[14].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[11]);
	pd3dRootParameters[14].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[15].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[15].Descriptor.ShaderRegister = 3; //SceneState
	pd3dRootParameters[15].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[15].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[16].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[16].Descriptor.ShaderRegister = 5; //ToLight
	pd3dRootParameters[16].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[16].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[17].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[17].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[17].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[12]);	// Depth Buffer
	pd3dRootParameters[17].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//pd3dRootParameters[20].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	//pd3dRootParameters[20].Constants.Num32BitValues = 2; //Time, ElapsedTime, xCursor, yCursor
	//pd3dRootParameters[20].Constants.ShaderRegister = 6; //Time
	//pd3dRootParameters[20].Constants.RegisterSpace = 0;
	//pd3dRootParameters[20].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[18].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[18].Constants.ShaderRegister = 6; //Time
	pd3dRootParameters[18].Constants.RegisterSpace = 0;
	pd3dRootParameters[18].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[19].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[19].Descriptor.ShaderRegister = 9; //Mesh Info(for UI Texture)
	pd3dRootParameters[19].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[19].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[20].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[20].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[20].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[13]);
	pd3dRootParameters[20].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dRootParameters[21].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[21].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[21].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[14]; //t27: gtxtParticleTexture
	pd3dRootParameters[21].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[22].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[22].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[22].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[15]; //t28: gtxtRandomTexture
	pd3dRootParameters[22].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[23].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pd3dRootParameters[23].DescriptorTable.NumDescriptorRanges = 1;
	pd3dRootParameters[23].DescriptorTable.pDescriptorRanges = &pd3dDescriptorRanges[16]; //t29: gtxtRandomOnSphereTexture
	pd3dRootParameters[23].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[24].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[24].Descriptor.ShaderRegister = 10; //Material
	pd3dRootParameters[24].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[24].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pd3dRootParameters[25].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pd3dRootParameters[25].Descriptor.ShaderRegister = 11; //Material
	pd3dRootParameters[25].Descriptor.RegisterSpace = 0;
	pd3dRootParameters[25].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[4];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = 0;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = 0;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[2].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	pd3dSamplerDescs[2].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[2].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[2].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[2].MipLODBias = 0;
	pd3dSamplerDescs[2].MaxAnisotropy = 16;
	pd3dSamplerDescs[2].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	pd3dSamplerDescs[2].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	pd3dSamplerDescs[2].MinLOD = 0;
	pd3dSamplerDescs[2].MaxLOD = 0;
	pd3dSamplerDescs[2].ShaderRegister = 2;
	pd3dSamplerDescs[2].RegisterSpace = 0;
	pd3dSamplerDescs[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[3].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	pd3dSamplerDescs[3].MipLODBias = 0.0f;
	pd3dSamplerDescs[3].MaxAnisotropy = 1;
	pd3dSamplerDescs[3].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[3].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
	pd3dSamplerDescs[3].MinLOD = 0;
	pd3dSamplerDescs[3].MaxLOD = 0;
	pd3dSamplerDescs[3].ShaderRegister = 3;
	pd3dSamplerDescs[3].RegisterSpace = 0;
	pd3dSamplerDescs[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	//D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_FLAGS d3dRootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_ALLOW_STREAM_OUTPUT;
	D3D12_ROOT_SIGNATURE_DESC d3dRootSignatureDesc;
	::ZeroMemory(&d3dRootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	d3dRootSignatureDesc.NumParameters = _countof(pd3dRootParameters);
	d3dRootSignatureDesc.pParameters = pd3dRootParameters;
	d3dRootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);
	d3dRootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	d3dRootSignatureDesc.Flags = d3dRootSignatureFlags;

	ID3DBlob* pd3dSignatureBlob = NULL;
	ID3DBlob* pd3dErrorBlob = NULL;
	auto a = D3D12SerializeRootSignature(&d3dRootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pd3dSignatureBlob, &pd3dErrorBlob);
	size_t rootSignatureSize = pd3dSignatureBlob->GetBufferSize();

	m_pd3dDevice->CreateRootSignature(0, pd3dSignatureBlob->GetBufferPointer(), pd3dSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&m_pd3dGraphicsRootSignature);

	if (pd3dSignatureBlob) pd3dSignatureBlob->Release();
	if (pd3dErrorBlob) pd3dErrorBlob->Release();
}

bool CGameFramework::CheckUIPopUp()
{	
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::TITLE) {

	}
	else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY) {
		if (CTextureShader::GetInstance()->IsSetting()) {
			CTextureShader::GetInstance()->SettingSwitch();
			return true;
		}
		else if (CTextureShader::GetInstance()->IsChannel()) {
			CTextureShader::GetInstance()->ChannelSwitch();
			return true;
		}
		else if (CTextureShader::GetInstance()->IsRandomShop()) {
			CTextureShader::GetInstance()->RandomShopSwitch();
			return true;
		}
		else if (CTextureShader::GetInstance()->IsAuction()) {
			CTextureShader::GetInstance()->AuctionSwitch();
			return true;
		}
		else if (CTextureShader::GetInstance()->IsBlockchain()) {
			CTextureShader::GetInstance()->BlockChainSwitch();
			return true;
		}
		else if (CTextureShader::GetInstance()->IsCustomize()) {
			CTextureShader::GetInstance()->CustomizeSwitch();
			return true;
		}
	}
	else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY) {

	}
	else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
		if (CTextureShader::GetInstance()->IsShopping()) {
			CTextureShader::GetInstance()->ShopSwitch();
			return true;
		}
	}
	

	return false;
}

void CGameFramework::OnDestroy()
{
    ReleaseObjects();
	SoundManager::GetInstance()->Release();

	::CloseHandle(m_hFenceEvent);

	if (m_pd3dDepthStencilBuffer) m_pd3dDepthStencilBuffer->Release();
	if (m_pd3dDsvDescriptorHeap) m_pd3dDsvDescriptorHeap->Release();

	for (int i = 0; i < m_nSwapChainBuffers; i++) if (m_ppd3dSwapChainBackBuffers[i]) m_ppd3dSwapChainBackBuffers[i]->Release();
	if (m_pd3dRtvDescriptorHeap) m_pd3dRtvDescriptorHeap->Release();

	if (m_pd3dCommandAllocator) m_pd3dCommandAllocator->Release();
	if (m_pd3dCommandQueue) m_pd3dCommandQueue->Release();
	if (m_pd3dCommandList) m_pd3dCommandList->Release();
	if (m_pd3dMTCommandList) m_pd3dMTCommandList->Release();
	if (m_pd3dMTCommandAllocator) m_pd3dMTCommandAllocator->Release();

	if (m_pShadowCamera)
	{
		m_pShadowCamera->Release();
		m_pShadowCamera = nullptr;
	}
	if (m_pShadowMappedCamera)
	{
		//delete m_pShadowMappedCamera;
		m_pShadowMappedCamera = nullptr;
	}

	if (m_pd3dFence) m_pd3dFence->Release();

	if (m_pd3dGraphicsRootSignature) m_pd3dGraphicsRootSignature->Release();

	if (m_pPipelineState) m_pPipelineState->Release();

	m_pdxgiSwapChain->SetFullscreenState(FALSE, NULL);
	if (m_pdxgiSwapChain) m_pdxgiSwapChain->Release();
    if (m_pd3dDevice) m_pd3dDevice->Release();
	if (m_pdxgiFactory) m_pdxgiFactory->Release();

	if (m_pBlurBuffer)m_pBlurBuffer->Release();

#if defined(_DEBUG)
	IDXGIDebug1	*pdxgiDebug = NULL;
	DXGIGetDebugInterface1(0, __uuidof(IDXGIDebug1), (void **)&pdxgiDebug);
	HRESULT hResult = pdxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_DETAIL);
	pdxgiDebug->Release();
#endif
}

void CGameFramework::BuildObjects()
{
	CreateGraphicsRootSignature();
	m_pUILayer = UILayer::Create(m_nSwapChainBuffers, 0, m_pd3dDevice, m_pd3dCommandQueue, m_ppd3dSwapChainBackBuffers, m_nWndClientWidth, m_nWndClientHeight);

	if (S_OK != m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL)) {
		cout << "CommandList Reset Fail" << endl;
	}
	
	if (S_OK != m_pd3dMTCommandList->Reset(m_pd3dMTCommandAllocator, NULL)) {
		cout << "CommandList Reset Fail" << endl;
	}

	CreateShaderVariables();

	//Frustum Create
	Frustum::GetInstance()->Create();

	m_pScene = new CTitleScene();
	if (m_pScene) m_pScene->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTitlePlayer *pPlayer = new CTitlePlayer(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);

	m_pScene->m_pPlayer = m_pPlayer = pPlayer;
	m_pCamera = m_pPlayer->GetCamera();

	m_pPostProcessingShader = new CPostProcessingShader();
	m_pPostProcessingShader->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pPostProcessingShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);

	D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	d3dRtvCPUDescriptorHandle.ptr += (::gnRtvDescriptorIncrementSize * m_nSwapChainBuffers);

	DXGI_FORMAT pdxgiResourceFormats[DEFERREDNUM] = { DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R32G32B32A32_FLOAT, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R8G8B8A8_UNORM };
	m_pPostProcessingShader->CreateResourcesAndViews(m_pd3dDevice, m_pd3dCommandList, DEFERREDNUM, pdxgiResourceFormats, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, d3dRtvCPUDescriptorHandle, DEFERREDNUM + 2); //SRV to (Render Targets) + (Depth Buffer)

	DXGI_FORMAT pdxgiDepthSrvFormats[1] = { DXGI_FORMAT_R32_FLOAT };
	m_pPostProcessingShader->CreateShaderResourceViews(m_pd3dDevice, 1, &m_pd3dDepthStencilBuffer, pdxgiDepthSrvFormats);

	m_pTextureShader = CTextureShader::Create();
	m_pTextureShader->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_pTextureShader->SetScene(SceneManager::GetInstance()->m_nCurScene);
	m_pTextureShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, NULL);
	
	SceneManager::GetInstance()->ReadFile();

	m_pDebugNormalToViewport = new CDebugNormalShader();
	m_pDebugNormalToViewport->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pDebugDiffuseToViewport = new CDebugDiffuseShader();
	m_pDebugDiffuseToViewport->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
	
	m_pDebugTextureToViewport = new CDebugTextureShader();
	m_pDebugTextureToViewport->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_pDebugLightingToViewport = new CDebugLightingShader();
	m_pDebugLightingToViewport->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);

	CTexture* pTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	m_pBlurBuffer = pTexture->CreateTexture(m_pd3dDevice, m_pd3dCommandList, 0, RESOURCE_TEXTURE2D, m_nWndClientWidth, m_nWndClientHeight,
		1, 1, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_COMMON, NULL);
	m_pBlurBuffer->AddRef();

	m_BlurShader = make_unique<CBlurShader>();
	m_BlurShader->CreateShader(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_BlurShader->BuildObjects(m_pd3dDevice, m_pd3dCommandList, m_pd3dGraphicsRootSignature, NULL, pTexture);

	Frustum::GetInstance()->m_xmfCamera4x4View = m_pCamera->GetViewMatrix(); 
	Frustum::GetInstance()->m_xmf4x4CameraProjection = m_pCamera->GetProjectionMatrix();
	Frustum::GetInstance()->Update();

	SoundManager::GetInstance()->Initialize();
	SoundManager::GetInstance()->Load_SoundFile("Sound/Archer/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/BGM/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Button/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Effect/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Fighter/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Npc/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Orge/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Programmer/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Structure/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/SwordMan/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/Wizzard/");
	SoundManager::GetInstance()->Load_SoundFile("Sound/");

	SoundManager::GetInstance()->Play_BGM(L"TitleBGM.ogg", 0.4f);

	if (S_OK != m_pd3dCommandList->Close()) {
		cout << "CommandList Close Fail" << endl;
	}
	
	if (S_OK != m_pd3dMTCommandList->Close()) {
		cout << "CommandList Close Fail" << endl;
	}
	ID3D12CommandList *ppd3dCommandLists[] = { m_pd3dCommandList, m_pd3dMTCommandList };
	m_pd3dCommandQueue->ExecuteCommandLists(2, ppd3dCommandLists);

	WaitForGpuComplete();

	if (m_pScene) m_pScene->ReleaseUploadBuffers();
	if (m_pPlayer) m_pPlayer->ReleaseUploadBuffers();

	m_GameTimer.Reset();
	//NetworkManager::GetInstance()->playerScene = SCENEKIND::TITLE;
}

void CGameFramework::ReleaseObjects()
{
	if (m_pUILayer) m_pUILayer->ReleaseResources();
	if (m_pUILayer) delete m_pUILayer;

	if (m_pPlayer) m_pPlayer->Release();

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) {
		delete m_pScene;
		m_pScene = nullptr;
	}

	if (m_pPostProcessingShader) m_pPostProcessingShader->ReleaseObjects();
	if (m_pPostProcessingShader) m_pPostProcessingShader->ReleaseShaderVariables();
	if (m_pPostProcessingShader) m_pPostProcessingShader->Release();
	m_pPostProcessingShader = nullptr;

	if (m_pTextureShader) m_pTextureShader->DestroyInstance();
	/*if (m_pTextureShader) m_pTextureShader->ReleaseObjects();
	if (m_pTextureShader) m_pTextureShader->Release();	*/
	
	if (m_pDebugNormalToViewport) m_pDebugNormalToViewport->ReleaseObjects();
	if (m_pDebugNormalToViewport) m_pDebugNormalToViewport->Release();

	if (m_pDebugDiffuseToViewport) m_pDebugDiffuseToViewport->ReleaseObjects();
	if (m_pDebugDiffuseToViewport) m_pDebugDiffuseToViewport->Release();

	if (m_pDebugTextureToViewport) m_pDebugTextureToViewport->ReleaseObjects();
	if (m_pDebugTextureToViewport) m_pDebugTextureToViewport->Release();

	if (m_pDebugLightingToViewport) m_pDebugLightingToViewport->ReleaseObjects();
	if (m_pDebugLightingToViewport) m_pDebugLightingToViewport->Release();

	if (m_pBBShader) m_pBBShader->ReleaseObjects();
	if (m_pBBShader) m_pBBShader->Release();
	
	if (m_pBlendShader) m_pBlendShader->ReleaseObjects();
	if (m_pBlendShader) m_pBlendShader->Release();

	if (m_pDissolveTexture)
		m_pDissolveTexture->Release();
}

void CGameFramework::ChangeSceneReleaseObject()
{
	if (m_pPlayer) {
		if (!m_pPlayer->Release())
			m_pPlayer = nullptr;
	}

	if (m_pScene) m_pScene->ReleaseObjects();
	if (m_pScene) {
		delete m_pScene;
		m_pScene = nullptr;
	}	

	if (m_pTextureShader) m_pTextureShader->DestroyInstance();

	if (m_pBBShader) m_pBBShader->ReleaseObjects();
	if (m_pBBShader) m_pBBShader->Release();

	if (m_pBlendShader) m_pBlendShader->ReleaseObjects();
	if (m_pBlendShader)
	{
		m_pBlendShader->Release();
		m_pBlendShader = nullptr;
	}

	if (m_pDissolveTexture) {
		m_pDissolveTexture->Release();
		m_pDissolveTexture = nullptr;
	}
}

void CGameFramework::ProcessInput()
{
	static UCHAR pKeysBuffer[256];
	bool bProcessedByScene = false;
	if (GetKeyboardState(pKeysBuffer) && m_pScene) bProcessedByScene = m_pScene->ProcessInput(pKeysBuffer);
	if (!bProcessedByScene)
	{
		float cxDelta = 0.0f, cyDelta = 0.0f;
		POINT ptCursorPos;
		/*if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME && !CTextureShader::GetInstance()->IsShopping())
		{
			::SetCapture(m_hWnd);
		}
		else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME && CTextureShader::GetInstance()->IsShopping()) {
			ReleaseCapture();
		}*/
		ReleaseCapture();

		if (GetCapture() == m_hWnd && SceneManager::GetInstance()->m_nCurScene != SCENEKIND::TITLE)
		{
			SetCursor(NULL);
			GetCursorPos(&ptCursorPos);
			cxDelta = (float)(ptCursorPos.x - m_ptOldCursorPos.x) / 3.0f;
			cyDelta = (float)(ptCursorPos.y - m_ptOldCursorPos.y) / 3.0f;
			SetCursorPos(m_ptOldCursorPos.x, m_ptOldCursorPos.y);
		}

		DWORD dwDirection = 0;
		if (pKeysBuffer[KEY_W] & 0xF0) dwDirection |= DIR_FORWARD;
		if (pKeysBuffer[KEY_S] & 0xF0) dwDirection |= DIR_BACKWARD;
		if (pKeysBuffer[KEY_A] & 0xF0) dwDirection |= DIR_LEFT;
		if (pKeysBuffer[KEY_D] & 0xF0) dwDirection |= DIR_RIGHT;
		if (pKeysBuffer[VK_PRIOR] & 0xF0) dwDirection |= DIR_UP;
		if (pKeysBuffer[VK_NEXT] & 0xF0) dwDirection |= DIR_DOWN;
		if (pKeysBuffer[KEY_T] & 0xF0) {
			switch (SceneManager::GetInstance()->m_nCurScene)
			{
			case SCENEKIND::TITLE:
				//SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat = true;
				break;
			case SCENEKIND::LOBBY:
				SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat = true;
				break;
			case SCENEKIND::READY:
				SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat = true;
				break;
			default:
				break;
			}
		}

		if ((dwDirection != 0) || (cxDelta != 0.0f) || (cyDelta != 0.0f))
		{
			if (cxDelta || cyDelta)
			{
				if (pKeysBuffer[VK_RBUTTON] & 0xF0)
					m_pPlayer->Rotate(cyDelta, 0.0f, -cxDelta); 
				else
					m_pPlayer->Rotate(cyDelta, cxDelta, 0.0f);
			}
			if (dwDirection) m_pPlayer->Move(dwDirection, PLAYER_SPEED, true);
		}
		else if (m_pPlayer->GetDir() != 0 && dwDirection == 0) {
			m_pPlayer->Move(dwDirection, 0.f, true);
		}
	}
	m_pPlayer->Update(m_GameTimer.GetTimeElapsed());
}

void CGameFramework::AnimateObjects()
{
	float fTimeElapsed = m_GameTimer.GetTimeElapsed();
	float animateTime = isShadowRender ? fTimeElapsed * 0.5f : fTimeElapsed;

	isShadowRender = SceneManager::GetInstance()->m_shadow;

	if (m_pScene) m_pScene->AnimateObjects(animateTime);

	//m_pPlayer->Animate(fTimeElapsed);
	if (m_pUILayer) //chatting cursor
	{
		if (NetworkManager::GetInstance()->playerScene == SCENEKIND::READY)
			m_pUILayer->CheckTime(fTimeElapsed);
		if (m_pUILayer->m_bChatting)
		{
			m_fCheckFilckerTime += fTimeElapsed;
			if (m_fCheckFilckerTime > 0.2f)
			{
				m_fCheckFilckerTime = 0;
				m_pUILayer->m_bFlicker = !m_pUILayer->m_bFlicker;
			}
		}
		m_pUILayer->SetElapseTime(fTimeElapsed);
	}
}

void CGameFramework::WaitForGpuComplete()
{
	const UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void CGameFramework::MoveToNextFrame()
{
	m_nSwapChainBufferIndex = m_pdxgiSwapChain->GetCurrentBackBufferIndex();

	UINT64 nFenceValue = ++m_nFenceValues[m_nSwapChainBufferIndex];
	HRESULT hResult = m_pd3dCommandQueue->Signal(m_pd3dFence, nFenceValue);

	if (m_pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = m_pd3dFence->SetEventOnCompletion(nFenceValue, m_hFenceEvent);
		::WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

//#define _WITH_PLAYER_TOP


void CGameFramework::FrameAdvance()
{
	try {
		// Checking for scene change
		if (SceneManager::GetInstance()->m_nCurScene != NetworkManager::GetInstance()->playerScene && NetworkManager::GetInstance()->playerScene != SCENEKIND::NONE) {
			if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::TITLE) {
				ChangeScene(SCENEKIND::LOBBY);
				SoundManager::GetInstance()->Play_BGM(L"LobbyBGM.wav", 0.2f);
				NetworkManager::GetInstance()->SendLoginCompletePacket();
			}
			else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY) {
				ChangeScene(SCENEKIND::READY);
				SoundManager::GetInstance()->Play_BGM(L"ReadyBGM.ogg", 1.4f);
				if (testing) {
					testing = false;

					if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) {
#ifdef WITH_DATABASE
						m_pPlayer->Customize({ 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 });
#else
						m_pPlayer->Customize({ 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1, 1 , 0, 1 , 3 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 });
#endif
					}
				}
				if (SceneManager::GetInstance()->GetOrder() == ORDER::BOSS) {
					NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(BOSSJOB::OGRE) + MAX_JOB);
				}
				else {
					NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(JOB::ARCHER));
				}
			}
			else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY) {
				ChangeScene(SCENEKIND::INGAME);
				SoundManager::GetInstance()->Play_BGM(L"IngameBGM.mp3", 0.1f);
				if (!testing) {
					NetworkManager::GetInstance()->SendLoadCompletePacket();
				}
				else {
#ifdef WITH_DATABASE
					NetworkManager::GetInstance()->m_ArrayInGameClientsCustom[0] = { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
#endif
					//NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name);
					NetworkManager::GetInstance()->SendJobSelectPacket(TEST_JOB);
					if (TEST_JOB >= 4) {
						for (int i = 0; i < 4; ++i) {
							NetworkManager::GetInstance()->SendSkillSelectPacket(i, NetworkManager::GetInstance()->readySceneInfo->selectSkills[3][i] - 1);
						}
					}
					else {
						for (int i = 0; i < 4; ++i) {
							NetworkManager::GetInstance()->SendSkillSelectPacket(i, NetworkManager::GetInstance()->readySceneInfo->selectSkills[0][i]);
						}
					}
				}
			}
			else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
				ChangeScene(SCENEKIND::LOBBY);
				SoundManager::GetInstance()->Play_BGM(L"LobbyBGM.wav", 0.2f);
			}
		}

		m_GameTimer.Tick(60.0f);

		switch (SceneManager::GetInstance()->m_nCurScene)
		{
		case SCENEKIND::TITLE:
			if (false == SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
				ProcessInput();
			break;
		case SCENEKIND::LOBBY:
			if (false == SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat)
				ProcessInput();
			break;
		case SCENEKIND::READY:
			if (false == SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat)
				ProcessInput();
			break;
		case SCENEKIND::INGAME:
			ProcessInput();
			break;
		default:
			break;
		}


		AnimateObjects();
		Update();

		HRESULT hResult = m_pd3dCommandAllocator->Reset();
		hResult = m_pd3dCommandList->Reset(m_pd3dCommandAllocator, NULL);

		D3D12_RESOURCE_BARRIER d3dResourceBarrier;
		::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
		d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
		d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		m_pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

		D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * ::gnRtvDescriptorIncrementSize);

		//float pfClearColor[4] = { 0.0f, 0.125f, 0.3f, 1.0f };
		//m_pd3dCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

		//

		//m_pd3dCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);


		if (m_pScene) m_pScene->OnPrepareRender(m_pd3dCommandList, m_pCamera, false);
		D3D12_VIEWPORT viewport = m_ShadowMap->Viewport();
		m_pd3dCommandList->RSSetViewports(1, &viewport);
		auto scissorRect = m_ShadowMap->ScissorRect();
		m_pd3dCommandList->RSSetScissorRects(1, &scissorRect);
		::SynchronizeResourceTransition(m_pd3dCommandList, m_ShadowMap->Resource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		m_pd3dCommandList->ClearDepthStencilView(m_ShadowMap->Dsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
		auto dsv = m_ShadowMap->Dsv();
		m_pd3dCommandList->OMSetRenderTargets(0, nullptr, false, &dsv);
		if (m_pPipelineState)m_pd3dCommandList->SetPipelineState(m_pPipelineState);

		XMFLOAT3 pos;
		XMFLOAT3 dir = XMFLOAT3(-0.5f, -0.7f, -0.5f);
		float radius = 60;

		XMFLOAT3 targetpos = m_pPlayer->GetPosition();
		XMVECTOR lightDir = XMLoadFloat3(&dir);
		XMVECTOR targetPos = XMLoadFloat3(&targetpos);
		XMVECTOR lightPos = targetPos - 2.0f * radius * lightDir;
		XMVECTOR lightUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
		XMMATRIX lightView = XMMatrixLookAtLH(lightPos, targetPos, lightUp);

		XMStoreFloat3(&pos, lightPos);

		// Transform bounding sphere to light space.
		XMFLOAT3 sphereCenterLS;
		XMStoreFloat3(&sphereCenterLS, XMVector3TransformCoord(targetPos, lightView));
		float l = sphereCenterLS.x - radius;
		float b = sphereCenterLS.y - radius;
		float n = sphereCenterLS.z - radius;
		float r = sphereCenterLS.x + radius;
		float t = sphereCenterLS.y + radius;
		float f = sphereCenterLS.z + radius;

		XMMATRIX lightProj = XMMatrixOrthographicOffCenterLH(l, r, b, t, n, f);
		XMMATRIX T(0.5f, 0.0f, 0.0f, 0.0f,
			0.0f, -0.5f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			0.5f, 0.5f, 0.0f, 1.0f);
		XMMATRIX S = lightView * lightProj * T;

		XMFLOAT4X4 proj;
		XMStoreFloat4x4(&proj, lightProj);

		XMFLOAT4X4 view;
		XMStoreFloat4x4(&view, lightView);

		XMStoreFloat4x4(&m_pShadowMappedCamera->m_xmf4x4View, XMMatrixTranspose(XMLoadFloat4x4(&view)));
		XMStoreFloat4x4(&m_pShadowMappedCamera->m_xmf4x4Projection, XMMatrixTranspose(XMLoadFloat4x4(&proj)));
		XMStoreFloat4x4(&m_pShadowMappedCamera->m_xm4x4ShadowTransform, XMMatrixTranspose(S));
		if (isShadowRender) XMStoreFloat4x4(&m_pCamera->GetCameraInfo()->m_xm4x4ShadowTransform, XMMatrixTranspose(S));

		::memcpy(&m_pShadowMappedCamera->m_xmf3Position, &pos, sizeof(XMFLOAT3));

		D3D12_GPU_VIRTUAL_ADDRESS d3dGPUVirtualAddress = m_pShadowCamera->GetGPUVirtualAddress();
		m_pd3dCommandList->SetDescriptorHeaps(1, &m_pScene->m_pd3dCbvSrvDescriptorHeap);
		m_pd3dCommandList->SetGraphicsRootConstantBufferView(0, d3dGPUVirtualAddress);


		if (m_pScene && isShadowRender) {

			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::READY)
				m_pScene->Render(m_pd3dCommandList, m_pCamera);
			else {
				m_pScene->m_ppHierarchicalGameObjects[0]->UpdateTransform(NULL);
				m_pScene->m_ppHierarchicalGameObjects[0]->Render(m_pd3dCommandList, m_pCamera, 0, -1);
			}

			if (m_pBlendShader)
			{
				for (int i = 0; i < m_pBlendShader->m_nObjects; ++i)
				{
					m_pBlendShader->m_ppObjects[i]->UpdateTransform(NULL);
					m_pBlendShader->m_ppObjects[i]->Render(m_pd3dCommandList, m_pCamera, 0, -1);
				}
			}
		}
		::SynchronizeResourceTransition(m_pd3dCommandList, m_ShadowMap->Resource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
		//::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);


		UpdateShaderVariables();
		if (m_pScene) m_pScene->OnPrepareRender(m_pd3dCommandList, m_pCamera);


		if (bDebugRendering)
		{
			m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList);
			//m_pCamera->UpdateShaderVariables(m_pd3dCommandList);
		}


		//if (bOneTime)

		D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		m_pPostProcessingShader->OnPrepareRenderTarget(m_pd3dCommandList, 1, &m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], d3dDsvCPUDescriptorHandle);
		if (m_pScene) m_pScene->Render(m_pd3dCommandList, m_pCamera);
#ifdef _WITH_PLAYER_TOP
		m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
#endif
		//if (m_pPlayer) m_pPlayer->Render(m_pd3dCommandList, m_pCamera);
		//if (m_pScene && m_pScene->m_pUIShader) m_pScene->m_pUIShader->Render(m_pd3dCommandList, m_pCamera);

		m_pPostProcessingShader->OnPostRenderTarget(m_pd3dCommandList);
		//if (m_pScene) m_pScene->OnPreRender(m_pd3dCommandList);

		//D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
		//m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
		m_pd3dCommandList->OMSetRenderTargets(1, &m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], TRUE, &d3dDsvCPUDescriptorHandle);
		m_pd3dCommandList->SetDescriptorHeaps(1, &m_pPostProcessingShader->m_pd3dCbvSrvDescriptorHeap);
		m_pd3dCommandList->SetGraphicsRootDescriptorTable(17, m_ShadowMap->Srv());
		m_pPostProcessingShader->Render(m_pd3dCommandList, m_pCamera);

		if (m_pScene)
			m_pScene->BillboardRender(m_pd3dCommandList, m_pCamera, m_pScene->GetDescriptorHeap());
		if (m_pBlendShader) {
			m_pBlendShader->PostRender(m_pd3dCommandList, m_pCamera, m_pScene->GetDescriptorHeap());
		}


		if (bDebugRendering)
		{
			D3D12_VIEWPORT d3dViewport2 = { 0.0f, 0.0f, FRAME_BUFFER_WIDTH * 0.25f, FRAME_BUFFER_HEIGHT * 0.25f, 0.0f, 1.0f };
			D3D12_RECT d3dScissorRect2 = { 0, 0, FRAME_BUFFER_WIDTH / 4, FRAME_BUFFER_HEIGHT / 4 };
			m_pd3dCommandList->RSSetViewports(1, &d3dViewport2);
			m_pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect2);

			if (m_pDebugNormalToViewport) m_pDebugNormalToViewport->Render(m_pd3dCommandList, m_pCamera);


			////2
			m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList);
			//m_pCamera->UpdateShaderVariables(m_pd3dCommandList);

			d3dViewport2 = { FRAME_BUFFER_WIDTH * 0.25f, 0.0f,FRAME_BUFFER_WIDTH * 0.25f, FRAME_BUFFER_HEIGHT * 0.25f, 0.0f, 1.0f };
			d3dScissorRect2 = { FRAME_BUFFER_WIDTH / 4, 0, FRAME_BUFFER_WIDTH / 2, FRAME_BUFFER_HEIGHT / 4 };
			m_pd3dCommandList->RSSetViewports(1, &d3dViewport2);
			m_pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect2);

			m_pd3dCommandList->SetDescriptorHeaps(1, &m_pPostProcessingShader->m_pd3dCbvSrvDescriptorHeap);
			m_pd3dCommandList->SetGraphicsRootDescriptorTable(17, m_ShadowMap->Srv());
			if (m_pDebugDiffuseToViewport) m_pDebugDiffuseToViewport->Render(m_pd3dCommandList, m_pCamera);

			////3
			m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList);
			//m_pCamera->UpdateShaderVariables(m_pd3dCommandList);

			d3dViewport2 = { FRAME_BUFFER_WIDTH * 0.5f, 0.0f,FRAME_BUFFER_WIDTH * 0.25f, FRAME_BUFFER_HEIGHT * 0.25f, 0.0f, 1.0f };
			d3dScissorRect2 = { FRAME_BUFFER_WIDTH / 2, 0, FRAME_BUFFER_WIDTH / 4 * 3, FRAME_BUFFER_HEIGHT / 4 };
			m_pd3dCommandList->RSSetViewports(1, &d3dViewport2);
			m_pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect2);

			if (m_pDebugTextureToViewport) m_pDebugTextureToViewport->Render(m_pd3dCommandList, m_pCamera);

			m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList);
			//m_pCamera->UpdateShaderVariables(m_pd3dCommandList);

			d3dViewport2 = { FRAME_BUFFER_WIDTH * 0.75f, 0.0f,FRAME_BUFFER_WIDTH * 0.25f, FRAME_BUFFER_HEIGHT * 0.25f, 0.0f, 1.0f };
			d3dScissorRect2 = { FRAME_BUFFER_WIDTH / 4 * 3, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT / 4 };
			m_pd3dCommandList->RSSetViewports(1, &d3dViewport2);
			m_pd3dCommandList->RSSetScissorRects(1, &d3dScissorRect2);

			if (m_pDebugLightingToViewport) m_pDebugLightingToViewport->Render(m_pd3dCommandList, m_pCamera);

			m_pCamera->SetViewportsAndScissorRects(m_pd3dCommandList);
			//m_pCamera->UpdateShaderVariables(m_pd3dCommandList);
		}
		Frustum::GetInstance()->m_iCountRender = 0;

		if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME)
			m_pScene->RenderParticle(m_pd3dCommandList, m_pCamera);

		if (!isBlurRender) {
			if (m_pScene)
				CTextureShader::GetInstance()->PostRender(m_pd3dCommandList, m_pCamera, m_pScene->GetDescriptorHeap());
		}
		else {

			// Render Target Resource Copy to BlurBuffer(For Blur Process)
			::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
			::SynchronizeResourceTransition(m_pd3dCommandList, m_pBlurBuffer, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);

			m_pd3dCommandList->CopyResource(m_pBlurBuffer, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex]);

			::SynchronizeResourceTransition(m_pd3dCommandList, m_pBlurBuffer, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
			::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
			m_pd3dCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);
			m_pd3dCommandList->ClearRenderTargetView(m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], Colors::Azure, 0, NULL);
			m_pd3dCommandList->OMSetRenderTargets(1, &m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBufferIndex], TRUE, &d3dDsvCPUDescriptorHandle);

			m_pd3dCommandList->SetDescriptorHeaps(1, &m_BlurShader->m_pd3dCbvSrvDescriptorHeap);
			m_BlurShader->Render(m_pd3dCommandList, m_pCamera);

			if (m_pScene)
				CTextureShader::GetInstance()->PostRender(m_pd3dCommandList, m_pCamera, m_pScene->GetDescriptorHeap());

		}
		::SynchronizeResourceTransition(m_pd3dCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);


		hResult = m_pd3dCommandList->Close();

		ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dCommandList };
		m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

		WaitForGpuComplete();

		if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME)
			m_pScene->OnPostRenderParticle();

		if (m_pUILayer)
		{
			UILayer::GetInstance()->Render(m_nSwapChainBufferIndex);
		}


#ifdef _WITH_PRESENT_PARAMETERS
		DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
		dxgiPresentParameters.DirtyRectsCount = 0;
		dxgiPresentParameters.pDirtyRects = NULL;
		dxgiPresentParameters.pScrollRect = NULL;
		dxgiPresentParameters.pScrollOffset = NULL;
		m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
		m_pdxgiSwapChain->Present(1, 0);
#else
		m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

		SoundManager::GetInstance()->Update_SoundManager();

		MoveToNextFrame();

		m_GameTimer.GetFrameRate(m_pszFrameRate + 12, 37);
		size_t nLength = _tcslen(m_pszFrameRate);
		XMFLOAT3 xmf3Position = m_pPlayer->GetPosition();
		_stprintf_s(m_pszFrameRate + nLength, 70 - nLength, _T("(%4f, %4f, %4f)"), xmf3Position.x, xmf3Position.y, xmf3Position.z);
		::SetWindowText(m_hWnd, m_pszFrameRate);
	}
	catch (exception ex) {
		cout << "Err: " << ex.what() << endl;
	}
}

DWORD WINAPI CGameFramework::ThreadProc(LPVOID lpParam)
{
	//CGameFramework* test = (CGameFramework*)lpParam;
	CGameFramework* test = reinterpret_cast<CGameFramework*>(lpParam);
	test->LoadingRender();

	SceneManager::GetInstance()->m_bWorkingThread = true;
	return 0;

}

void CGameFramework::LoadingRender()
{
	try {
		while (SceneManager::GetInstance()->m_bWorkingThread)
		{
			m_GameTimer.Tick(60.0f);

			HRESULT hResult = m_pd3dMTCommandAllocator->Reset();
			hResult = m_pd3dMTCommandList->Reset(m_pd3dMTCommandAllocator, NULL);

			D3D12_RESOURCE_BARRIER d3dResourceBarrier;
			::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
			d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			d3dResourceBarrier.Transition.pResource = m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex];
			d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
			d3dResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
			d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			m_pd3dMTCommandList->ResourceBarrier(1, &d3dResourceBarrier);

			D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle = m_pd3dRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			d3dRtvCPUDescriptorHandle.ptr += (m_nSwapChainBufferIndex * ::gnRtvDescriptorIncrementSize);

			float pfClearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
			m_pd3dMTCommandList->ClearRenderTargetView(d3dRtvCPUDescriptorHandle, pfClearColor/*Colors::Azure*/, 0, NULL);

			D3D12_CPU_DESCRIPTOR_HANDLE d3dDsvCPUDescriptorHandle = m_pd3dDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_pd3dMTCommandList->ClearDepthStencilView(d3dDsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

			m_pd3dMTCommandList->OMSetRenderTargets(1, &d3dRtvCPUDescriptorHandle, TRUE, &d3dDsvCPUDescriptorHandle);

			::SynchronizeResourceTransition(m_pd3dMTCommandList, m_ppd3dSwapChainBackBuffers[m_nSwapChainBufferIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);

			hResult = m_pd3dMTCommandList->Close();

			ID3D12CommandList* ppd3dCommandLists[] = { m_pd3dMTCommandList };
			m_pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

			WaitForGpuComplete();



			if (m_pUILayer)
			{
				if (m_pUILayer->GetLerpProgressing() >= 1.f && m_pUILayer->GetLoadingPreProgressPercent() >= 1.f)
				{
					SceneManager::GetInstance()->m_bWorkingThread = false;
					m_pUILayer->ResetLoadingValue();
					SceneManager::GetInstance()->m_fLoadingProgressPercent = 0.f;
				}

				bool bLoading = SceneManager::GetInstance()->m_bWorkingThread;
				if (bLoading)
					UILayer::GetInstance()->Render(m_nSwapChainBufferIndex, bLoading);
			}

#ifdef _WITH_PRESENT_PARAMETERS
			DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
			dxgiPresentParameters.DirtyRectsCount = 0;
			dxgiPresentParameters.pDirtyRects = NULL;
			dxgiPresentParameters.pScrollRect = NULL;
			dxgiPresentParameters.pScrollOffset = NULL;
			m_pdxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);
#else
#ifdef _WITH_SYNCH_SWAPCHAIN
			m_pdxgiSwapChain->Present(1, 0);
#else
			m_pdxgiSwapChain->Present(0, 0);
#endif
#endif

			MoveToNextFrame();
		}
	}
	catch (exception ex) {
		cout << "Loading Render err: " << ex.what() << endl;
	}
}

void CGameFramework::Resize(int width, int height)
{


}
