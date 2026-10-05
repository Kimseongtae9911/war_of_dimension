//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"
#include "Player.h"
#include "ObjectConstantArena.h"


struct LIGHT
{
	XMFLOAT4							m_xmf4Ambient;
	XMFLOAT4							m_xmf4Diffuse;
	XMFLOAT4							m_xmf4Specular;
	XMFLOAT3							m_xmf3Position;
	float 								m_fFalloff;
	XMFLOAT3							m_xmf3Direction;
	float 								m_fTheta; //cos(m_fTheta)
	XMFLOAT3							m_xmf3Attenuation;
	float								m_fPhi; //cos(m_fPhi)
	bool								m_bEnable;
	int									m_nType;
	float								m_fRange;
	float								padding;
};										
										
struct LIGHTS							
{										
	LIGHT								m_pLights[MAX_LIGHTS];
	XMFLOAT4							m_xmf4GlobalAmbient;
	int									m_nLights;
};

struct VS_CB_SCENE_STATE
{
	UINT m_nDrawOption;
	float m_fExposure, m_fSaturation, m_fContrast, m_fVibrance;
	UINT m_nCurScene;
	bool m_outline = true;
	//XMFLOAT4 m_xmf4SceneColorGradingParameter;
};

class CScene
{
public:
    CScene();
    ~CScene();

	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel);

	virtual void ReleaseObjects();

	bool ProcessInput(UCHAR *pKeysBuffer);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bIsAnimate = true);
	virtual void RenderParticle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void OnPostRenderParticle();
    std::shared_ptr<ParticleBufferPool> m_particleBufferPool;
    void SubmitParticleFrame(ID3D12CommandQueue* queue) { if (m_particleBufferPool) m_particleBufferPool->SubmitFrame(queue); }
    std::shared_ptr<ObjectConstantArena> m_objectConstantArena;
    void BeginObjectConstantFrame(UINT frame) { if (m_objectConstantArena) m_objectConstantArena->BeginFrame(frame); }
    void SubmitObjectConstantFrame(ID3D12CommandQueue* queue) { if (m_objectConstantArena) m_objectConstantArena->SubmitFrame(queue); }

	virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimate = true);
	virtual void OnMTPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList);

	virtual void ReleaseUploadBuffers();

	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pd3dCbvSrvDescriptorHeap; }

	virtual void BillboardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap) {}

	ID3D12RootSignature* GetRootSignature() { return m_pd3dGraphicsRootSignature; }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorNextHandle() { return(m_d3dSrvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorNextHandle() { return(m_d3dSrvGPUDescriptorNextHandle); }

	CPlayer								*m_pPlayer = NULL;
	CGameObject** m_ppOtherClient = NULL;

	static ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap;

protected:
	ID3D12RootSignature					*m_pd3dGraphicsRootSignature = NULL;


	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorStartHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorStartHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorStartHandle;

	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dCbvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dCbvGPUDescriptorNextHandle;
	static D3D12_CPU_DESCRIPTOR_HANDLE	m_d3dSrvCPUDescriptorNextHandle;
	static D3D12_GPU_DESCRIPTOR_HANDLE	m_d3dSrvGPUDescriptorNextHandle;	

public:
	static void CreateCbvSrvDescriptorHeaps(ID3D12Device *pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);

	static D3D12_GPU_DESCRIPTOR_HANDLE CreateConstantBufferViews(ID3D12Device *pd3dDevice, int nConstantBufferViews, ID3D12Resource *pd3dConstantBuffers, UINT nStride);
	static void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);

	

	float								m_fElapsedTime = 0.0f;

	int									m_nGameObjects = 0;
	CGameObject							**m_ppGameObjects = NULL;

	int									m_nHierarchicalGameObjects = 0;
	CGameObject							**m_ppHierarchicalGameObjects = NULL;

	int									m_nShaders = 0;
	CShader								**m_ppShaders = NULL;

	CSkyBox								*m_pSkyBox = NULL;
	
	unordered_map<PARTICLE_SITUATION, array<vector<CParticleObject*>, 5>> m_ParticleObjects;
	unordered_map<SKILL_TYPE, vector<CParticleObject*>> m_SkillTypeParticle;

	LIGHT								*m_pLights = NULL;
	int									m_nLights = 0;

	XMFLOAT4							m_xmf4GlobalAmbient;

	ID3D12Resource						*m_pd3dcbLights = NULL;
	LIGHTS								*m_pcbMappedLights = NULL;

	ID3D12Resource* m_pd3dcbScene = NULL;
	VS_CB_SCENE_STATE* m_pcbMappedScene = NULL;
	UINT m_nDrawOption = HABLE_MACCANN;
	float m_fExposure = 1.1f, m_fSaturation = 1.2f, m_fContrast = 1.2f, m_fVibrance = 1.6f;
	UINT m_nCurScene = static_cast<UINT>(SCENEKIND::TITLE);
	bool m_outline = true;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CTitleScene : public CScene
{
public:
	CTitleScene();
	virtual ~CTitleScene();

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bIsAnimate = true);
	//virtual void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CLobbyScene : public CScene
{
public:
	CLobbyScene();
	virtual ~CLobbyScene();

	virtual void ReleaseObjects();

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bIsAnimate = true);
	virtual void AnimateObjects(float fTimeElapsed);

	virtual void BillboardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap);
	virtual void ReleaseUploadBuffers();
protected:
	CLobbyNpc** m_ppNpcs = NULL;
	CLobbyNpc* m_pNearNpc = NULL;
	const XMFLOAT3 m_npcPositions[LOBBY_NPC] = {
		XMFLOAT3(2.552067f, 2.637576f, -6.728072f),
		XMFLOAT3(-19.102850f, 2.360098f,23.500757f),
		XMFLOAT3(-16.562815f,6.315217f,-19.624289f),
		XMFLOAT3(-19.408022f, 3.0709f, -57.537098f)
	};

	CBillboardUIShader* m_BillboardShader = NULL;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CReadyScene : public CScene
{
public:
	CReadyScene();
	virtual ~CReadyScene();

	void BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel) override;

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature);
	virtual void ReleaseObjects();

	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bIsAnimate = true);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CIngameScene : public CScene
{
public:
	CIngameScene();
	virtual ~CIngameScene();

	virtual void ReleaseObjects();
	void ReleaseParticles();

	void BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel) override;

public:
	virtual bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	virtual bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	virtual void BuildDefaultLightsAndMaterials();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandLis, ID3D12RootSignature* pd3dGraphicsRootSignature);

	virtual void ReleaseUploadBuffers();

	virtual void AnimateObjects(float fTimeElapsed);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, bool bIsAnimate = true);
	virtual void BillboardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap);



public:
	CGameObject** m_minions = NULL;
	CGameObject** m_monsters = NULL;
	CGameObject** m_towerAttacks = NULL;

	CBillboardUIShader* m_BillboardShader = NULL;

	unordered_map<SKILL_TYPE, vector<CSkillObject*>> m_skillObjects;

	int			m_iParticleTextureNum = 0;
	CTexture**  m_pParticleTexture = nullptr;
	static const wchar_t* ParticleTextureAddress[ADDRESS_COUNT];
};
