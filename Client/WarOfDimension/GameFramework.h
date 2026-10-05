#pragma once

// Key Code
#define KEY_W 0x57
#define KEY_S 0x53
#define KEY_A 0x41
#define KEY_D 0x44

#include "Timer.h"
#include "Player.h"
#include "Scene.h"
#include "UILayer.h"
#include "ShadowMap.h"
#include "CBlurShader.h"

struct TIME
{
	float fCurrentTime, fElapsedTime;
};

class CGameFramework
{
public:
	CGameFramework();
	~CGameFramework();

	bool OnCreate(HINSTANCE hInstance, HWND hMainWnd);
	void OnDestroy();

	void CreateSwapChain();
	void CreateDirect3DDevice();
	void CreateCommandQueueAndList();

	void CreateRtvAndDsvDescriptorHeaps();

	void CreateRenderTargetViews();
	void CreateDepthStencilView();

	void CreateShadowMap();
	void CreateShadowMapCamera();

	void ChangeSwapChainState();

    void BuildObjects();
    void ReleaseObjects();
	void ChangeSceneReleaseObject();

    void ProcessInput();
    void AnimateObjects();
    void FrameAdvance();

	void WaitForGpuComplete();
	void MoveToNextFrame();

	void OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	void OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	LRESULT CALLBACK OnProcessingWindowMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void ChangeScene(SCENEKIND nSceneKind);
	void ProfileMemorySnapshot(const char* phase);
	void ProfileReleaseParticles();
    void ProfileParticleReuseFrames();
	int AuditHeroSelection(const wchar_t* reportPath);
	void CaptureMonsters(const wchar_t* directory);
	void Update();

	void CreateShaderVariables();
	void UpdateShaderVariables();

	static DWORD WINAPI ThreadProc(LPVOID lpParam);
	void LoadingRender();

	void Resize(int width, int height);

private:
	void CreateGraphicsRootSignature();
	bool CheckUIPopUp();

private:
	HINSTANCE					m_hInstance;
	HWND						m_hWnd; 
	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

	int							m_nWndClientWidth;
	int							m_nWndClientHeight;
        
	IDXGIFactory4				*m_pdxgiFactory = NULL;
	IDXGISwapChain3				*m_pdxgiSwapChain = NULL;
	ID3D12Device				*m_pd3dDevice = NULL;

	bool						m_bMsaa4xEnable = false;
	UINT						m_nMsaa4xQualityLevels = 0;

	static const UINT			m_nSwapChainBuffers = 2;
	UINT						m_nSwapChainBufferIndex;

	ID3D12Resource				*m_ppd3dSwapChainBackBuffers[m_nSwapChainBuffers];
	ID3D12DescriptorHeap		*m_pd3dRtvDescriptorHeap = NULL;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_pd3dSwapChainBackBufferRTVCPUHandles[m_nSwapChainBuffers];

	ID3D12Resource				*m_pd3dDepthStencilBuffer = NULL;
	ID3D12DescriptorHeap		*m_pd3dDsvDescriptorHeap = NULL;
	D3D12_CPU_DESCRIPTOR_HANDLE m_DSVDescriptorCPUHandle;

	ID3D12CommandAllocator		*m_pd3dCommandAllocator = NULL;
	ID3D12CommandQueue			*m_pd3dCommandQueue = NULL;
	ID3D12GraphicsCommandList	*m_pd3dCommandList = NULL;

	ID3D12GraphicsCommandList* m_pd3dMTCommandList = NULL;
	ID3D12CommandAllocator* m_pd3dMTCommandAllocator = NULL;

	ID3D12Fence					*m_pd3dFence = NULL;
	UINT64						m_nFenceValues[m_nSwapChainBuffers];
	HANDLE						m_hFenceEvent;

	SCENEKIND					m_nCurScene = SCENEKIND::TITLE;

#if defined(_DEBUG)
	ID3D12Debug					*m_pd3dDebugController;
#endif

	CGameTimer					m_GameTimer;

	CScene						*m_pScene = NULL;
	CPlayer						*m_pPlayer = NULL;
	CCamera						*m_pCamera = NULL;

	UILayer						*m_pUILayer = NULL;

	POINT						m_ptOldCursorPos;

	_TCHAR						m_pszFrameRate[70];

	CPostProcessingShader* m_pPostProcessingShader = NULL;

	CTextureShader* m_pTextureShader = NULL;	

	CDebugNormalShader* m_pDebugNormalToViewport = NULL;
	CDebugDiffuseShader* m_pDebugDiffuseToViewport = NULL;
	CDebugTextureShader* m_pDebugTextureToViewport = NULL;
	CDebugLightingShader* m_pDebugLightingToViewport = NULL;
	
	CBoundingBoxShader* m_pBBShader = NULL;

	CBlendObjectShader* m_pBlendShader = NULL;

	bool bDebugRendering = false;

	float m_fCheckFilckerTime = 0;

	bool testing = false;

	CTexture* m_pDissolveTexture = NULL;

	///
	CCamera* m_pViewCamera = NULL;

	//cbv
	ID3D12Resource* m_pd3dcbTime = NULL;
	TIME* m_pTime = NULL;

	//Shadow
	unique_ptr<ShadowMap> m_ShadowMap = NULL;
	ID3D12PipelineState* m_pPipelineState;
	ID3D12Resource* m_pShadowCamera = NULL;
	VS_CB_CAMERA_INFO* m_pShadowMappedCamera = NULL;
	bool isShadowRender = true;

	//Blur
	unique_ptr<CBlurShader> m_BlurShader = NULL;
	//For Blur... Added More Buffers more Post Process Render
	ID3D12Resource* m_pBlurBuffer = NULL;
	bool isBlurRender = true;

	bool m_isExecutedOnce = false;
};

