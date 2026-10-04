//-----------------------------------------------------------------------------
// File: Shader.h
//-----------------------------------------------------------------------------

#pragma once

#include "Object.h"
#include "Camera.h"
#include "UILayer.h"

struct LIGHT;

class CShader
{
public:
	CShader();
	virtual ~CShader();

private:
	int									m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	D3D12_SHADER_BYTECODE ReadCompiledShaderFromFile(const WCHAR *pszFileName, ID3DBlob **ppd3dShaderBlob=NULL);

	virtual void CreateShader(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) { }
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList) { }
	virtual void ReleaseShaderVariables() { }

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World) { }

	virtual void OnPrepareRender(ID3D12GraphicsCommandList *pd3dCommandList, int nPipelineState=0);
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int m_nPipelineStates = 0);

	virtual void ReleaseUploadBuffers() { }

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CLoadedModelInfo *pModel, void *pContext = NULL) { }
	virtual void AnimateObjects(float fTimeElapsed) { }
	virtual void ReleaseObjects() { }

	CTexture* LoadTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* filePath);
	CTexturedRectMesh* CreateTexturedRectMesh(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float width, float height);

	void CreateCbvSrvDescriptorHeaps(ID3D12Device* pd3dDevice, int nConstantBufferViews, int nShaderResourceViews);
	void CreateConstantBufferViews(ID3D12Device* pd3dDevice, int nConstantBufferViews, ID3D12Resource* pd3dConstantBuffers, UINT nStride);
	void CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex);
	void CreateShaderResourceViews(ID3D12Device* pd3dDevice, int nResources, ID3D12Resource** ppd3dResources, DXGI_FORMAT* pdxgiSrvFormats);

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart()); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandleForHeapStart() { return(m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUCbvDescriptorStartHandle() { return(m_d3dCbvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUCbvDescriptorStartHandle() { return(m_d3dCbvGPUDescriptorStartHandle); }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorStartHandle() { return(m_d3dSrvCPUDescriptorStartHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorStartHandle() { return(m_d3dSrvGPUDescriptorStartHandle); }	
	
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUSrvDescriptorNextHandle() { return(m_d3dSrvCPUDescriptorNextHandle); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSrvDescriptorNextHandle() { return(m_d3dSrvGPUDescriptorNextHandle); }

	ID3D12DescriptorHeap* GetDescriptorHeap() { return m_pd3dCbvSrvDescriptorHeap; }

	void SetScene(SCENEKIND nSceneKind) { m_CurScene = nSceneKind; }
	SCENEKIND GetScene() { return m_CurScene; }

	ID3D12DescriptorHeap* m_pd3dCbvSrvDescriptorHeap = NULL;
	ID3D12RootSignature* m_pd3dGraphicsRootSignature = NULL;

protected:
	ID3DBlob							*m_pd3dVertexShaderBlob = NULL;
	ID3DBlob							*m_pd3dPixelShaderBlob = NULL;
	ID3DBlob							*m_pd3dGeometryShaderBlob = NULL;

	ID3D12PipelineState					**m_ppd3dPipelineStates = NULL;
	int									m_nPipelineStates = 0;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC	m_d3dPipelineStateDesc;

	float								m_fElapsedTime = 0.0f;



	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE		m_d3dSrvCPUDescriptorNextHandle;  
	D3D12_GPU_DESCRIPTOR_HANDLE		m_d3dSrvGPUDescriptorNextHandle; 
	SCENEKIND							m_CurScene = SCENEKIND::NONE;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBoxShader : public CShader
{
public:
	CSkyBoxShader();
	virtual ~CSkyBoxShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CStandardShader : public CShader
{
public:
	CStandardShader();
	virtual ~CStandardShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	//virtual D3D12_BLEND_DESC CreateBlendState();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

protected:
	DXGI_FORMAT deferredBufferFormats[DEFERREDNUM + 1] =
	{
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R32G32B32A32_FLOAT,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM,
		DXGI_FORMAT_R8G8B8A8_UNORM
	};
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkinnedAnimationStandardShader : public CStandardShader
{
public:
	CSkinnedAnimationStandardShader();
	virtual ~CSkinnedAnimationStandardShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	//virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkinnedAnimationObjectsShader : public CSkinnedAnimationStandardShader
{
public:
	CSkinnedAnimationObjectsShader();
	virtual ~CSkinnedAnimationObjectsShader();

	virtual void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CLoadedModelInfo *pModel, void *pContext = NULL);
	virtual void AnimateObjects(float fTimeElapsed);
	virtual void ReleaseObjects();

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int nPipelineState = 0);

public:
	CGameObject						**m_ppObjects = 0;
	int								m_nObjects = 0;
};

/////////////////////////////////////////////////////////////////////////////////////
struct PS_CB_DRAW_OPTIONS
{
	XMINT4							m_xmn4DrawOptions;
};

class CPostProcessingShader : public CShader
{
public:
	CPostProcessingShader();
	virtual ~CPostProcessingShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState=0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	//virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);
	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);


	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0); //Deferred

	void ForwardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);


	void CreateResourcesAndViews(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nResources, DXGI_FORMAT* pdxgiFormats, UINT nWidth, UINT nHeight, D3D12_CPU_DESCRIPTOR_HANDLE d3dRtvCPUDescriptorHandle, UINT nShaderResources);

	void OnPrepareRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList, int nRenderTargets, D3D12_CPU_DESCRIPTOR_HANDLE* pd3dRtvCPUHandles, D3D12_CPU_DESCRIPTOR_HANDLE d3dDepthStencilBufferDSVCPUHandle);
	void OnPostRenderTarget(ID3D12GraphicsCommandList* pd3dCommandList);

protected:
	CTexture* m_pTexture = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE* m_pd3dRtvCPUDescriptorHandles = NULL;

public:
	CTexture* GetTexture() { return(m_pTexture); }
	ID3D12Resource* GetTextureResource(UINT nIndex) { return(m_pTexture->GetResource(nIndex)); }

	D3D12_CPU_DESCRIPTOR_HANDLE GetRtvCPUDescriptorHandle(UINT nIndex) { return(m_pd3dRtvCPUDescriptorHandles[nIndex]); }

protected:
	CGameObject** m_ppObjects = 0;
	int								m_nObjects = 0;

};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct VS_VB_BILLBOARD_INSTANCE
{
	XMFLOAT3						m_xmf3Position;
	XMFLOAT4						m_xmf4BillboardInfo;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// UI Texture Render
class CTextureShader : public CStandardShader
{
public:
	CTextureShader();
	virtual ~CTextureShader();

	static CTextureShader* Create();
	HRESULT Initialize();
	void Reset();

	static CTextureShader* GetInstance() { return s_instance; }
	static void DestroyInstance();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	void ResizeWindow(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, float fResolution);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);
	virtual void ReleaseObjects();
	void PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap);

	virtual void ReleaseUploadBuffers();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	void SetReadySceneSkillUI(bool b);
	bool GetReadySceneSkillUI() { return m_bReadySceneSKillUI; }

	int SetSkillArray(int SkillNumber, ORDER ePos);
	void SetSkillArrayReset(int ArrayNum, ORDER ePos);
	void SetSkillServer(array<int,4> Skill, ORDER epos);
	void SetPlayerSkillServerIngame(array<int, 4> Skill) {
		for (int i = 0; i < 4; ++i) {
			m_iClientSkillArray[i] = Skill[i] - PLAYER_SKILL / 4;
	} }
	void SetBossSkillServerIngame(array<int, 4> Skill) { for (int i = 0; i < 4; ++i) { m_iClientSkillArray[i] = Skill[i] - BOSS_SKILL / 2 - 1; } }

	void SetPlayerJOB(JOB eClient, ORDER ePos) { m_ePlayerJOB[static_cast<int>(ePos)] = eClient; }
	JOB GetPlayerJOB(ORDER eOrder) { return  m_ePlayerJOB[static_cast<int>(eOrder)]; }

	bool ReadyPlayer(ORDER ePos);

	void SetBossJob(BOSSJOB bJob) { m_eBossJOB = bJob; }
	BOSSJOB GetBossJob() { return m_eBossJOB; }

	void Click(int objNum, bool click) { m_ppObjects[objNum]->m_pMesh->Click(click); }
	void OnMouseMoved(float mouseX, float mouseY);
	void OnMouseClick();
	void OnMouseRelease();

	bool isRenderStats();
	bool isRenderMinimap();
	XMFLOAT2 WorldToMinimap(XMFLOAT2 pos, XMFLOAT2 iconSize);
	void MinimapSwitch();

	bool isRenderShop();
	int GetPrice(int index);
	int GetStatLevel(int stat); 
	void ShopSwitch();
	bool IsShopping() const { return m_shopping; }

	void SetItemState(int index, bool state, ITEMKIND item = ITEMKIND::NONE);

	void SettingSwitch(); 
	bool IsSetting() const;
	void SettingReset();

	void MouseMoveLoop(CUIObject** objects, int max, float mouseX, float mouseY);

	void AddVolume() { m_masterVolume += m_masterVolume < 100 ? 1 : 0; }
	void SubtractVolume() { m_masterVolume -= m_masterVolume > 0 ? 1 : 0; }
	int GetVolume() const { return m_masterVolume; }

	void ChannelSwitch();
	bool IsChannel() const { return m_channel; }

	void InteractionSwitch(bool interaction);
	void AwayFromNpc();

	bool IsRandomShop() const { return m_randomShop; }
	int GetCurParts() const { return m_curPartsNum; }
	void RandomShopSwitch();
	void QuestionIconSwitch();
	void AddCurParts();
	void SubtractCurParts();
	void RandomStart(); 
	bool RandomGenParts(float elapsedTime);

	XMFLOAT2 GetCustomizeUvOffset(int kind, int num);
	
	bool IsAuction() const { return m_auction; }
	void AuctionSwitch();
	void AuctionProductOff(int n);
	void AuctionProductOn(int n);
	LobbyAuctionInfo* m_auctionInfo = nullptr;

	bool IsBlockchain() const { return m_blockchain; }
	void BlockChainSwitch();
	void SetinputKind(bool period) { m_inputPeriod = period; }
	bool GetinputKind()const { return m_inputPeriod; }
	LobbyBlockChainInfo* m_blockChainInfo = nullptr;

	bool IsCustomize() const { return m_customize; }
	void CustomizeSwitch();
	int m_curParts = 14;	// for customize
	int m_nCurParts = 0;	// for customize
	int m_indexCustomize = 0; // for customize

	void SetSkillInfoTexture(ORDER order, int num);
	void SetSkillInfoTexturePos(int mouseX, int mouseY);

	void AnimateSpeedTexture(float elapsedTime);
	void AccelerateTextureSwitch(bool accelerate) { m_accelerate = accelerate; }
	void AnimateGetDamagedTexture(float elapsedTime);
	void GetDamage() { m_getDamaged = true;  m_getDamagedTextureLifetime = 0.1f; }

	void Win() { m_pWinTexture->DrawOn(); }
	void Defeat() { m_pDefeatTexture->DrawOn(); }
protected:
	CUIObject**						m_ppObjects = 0;
	int								m_nObjects = 0;	
	
	CUIObject**						m_ppUITextures = 0;
	int								m_nUITextures = 0;

	CUIObject**						m_ppExtraTextures = 0;
	int								m_nExtraTextures = 0;

	vector<CUIObject*>				m_shopTextures;
	int								m_nShopTextures = 0;

	vector<CUIObject*>				m_ItemTextures;

	vector<CUIObject*>				m_channelTextures;
	int								m_nChannelTextures = 0;

	vector<CUIObject*>				m_auctionTextures;
	int								m_nAuctionTextures = 0;

	vector<CUIObject*>				m_blockchainTextures;
	int								m_nBlockchainTextures = 0;

	vector<CUIObject*>				m_customizeTextures;
	int								m_nCustomizeTextures = 0;

	CUIObject* m_pHoveredObject = nullptr;
	
	// Ingame Effects
	CUIObject* m_pAccelerateObject = nullptr;
	CUIObject* m_pGetDamageObject = nullptr;
	CUIObject* m_pWinTexture = nullptr;
	CUIObject* m_pDefeatTexture = nullptr;

	//Ready
	CUIObject* m_pSkillInfoTexture = nullptr;

	//Lobby
	bool m_setting = false;
	bool m_channel = false;
	bool m_randomShop = false;
	bool m_auction = false;
	bool m_blockchain = false;
	bool m_customize = false;
	int m_masterVolume = 50;
	int m_curPartsNum = 0;	//부위
	int m_curWonPart = 0; // 획득한 파츠
	float m_randomTime = 0.8f;
	bool m_startRandom = false;
	bool m_inputPeriod = false; // true == 기간 입력, false == 토큰 입력

	//Ready
	bool							m_bReadySceneSKillUI = false;
	int m_iPlayerSkillArrays[4][4];

	JOB								m_ePlayerJOB[3];
	BOSSJOB							m_eBossJOB = BOSSJOB::OGRE;

	//Ingame
	int								m_iClientSkillArray[4];
	bool m_shopping = false;
	float m_speedTime = 0.1f;
	int m_curSpeedUOffset = 0;
	float m_getDamagedTextureLifetime = 0.3f;
	bool m_getDamaged = false;
	bool m_accelerate = false;
protected:
	//Lobby Constant Value
	//for setting
	const int NumSetting = 1;
	const int NumCheckBox = 9; // HDR 5 + Outline 2 + Shadow 2
	const int NumCheckIcon = 3; // hdr + outline + shadow
	const int NumPlusButton = 5;
	const int NumMinusButton = 5;
	const int NumFullScreen = 1;
	const int NumWindowMode = 1;
	const int NumAccept = 1;
	const int NumReset = 1;
	//for Auction
	const int NumAuction = 1;
	const int NumLeftIcon = 3;
	const int NumRightIcon = 3;
	const int NumCalc = 12; // Use blockchain too
	const int NumMyPart = 1;
	const int NumProduct = 7;
	const int NumRegist = 1;
	const int NumQuit = 1;// Use blockchain, Customize too
	//for blockcahin
	const int NumBlockChain = 1;
	const int NumInputBar = 2;
	const int NumStaking = 1;
	//for Customize
	const int NumCustomize = 1;
	const int NumLeftArrow = 2;
	const int NumRightArrow = 2;
	const int NumItemParts = 1;
	const int NumApply = 1;
	const int NumLiftUp = 1;

	//Ingame Constant Value
	//for minimap
	const int NumMinimaps = 1;
	const int NumNexus = 1;
	const int NumTowers = 4;
	const int NumTeleports = 2;
	const int NumUnique = 1;
	const int NumBlueCircles = 3;
	const int NumRedCircles = 1;
	//for shop
	const int NumShop = 1;
	const int NumPurchaseLong = 11;
	const int NumPurchaseShort = 9;
	const int NumExit = 1;
	//for ui
	const int NumItem = 5;

private:
	static CTextureShader* s_instance;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//// billboard Texture Render
class CBillboardUIShader : public CStandardShader
{
public:
	CBillboardUIShader();
	CBillboardUIShader(CGameObject** ppMonsters, CGameObject** ppMinions, CGameObject** ppOtherClients);
	virtual ~CBillboardUIShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);


	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState();

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);
	virtual void ReleaseObjects();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);
	void PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap);

	virtual void ReleaseUploadBuffers();

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

protected:
	CGameObject** m_ppObjects = 0;
	CGameObject** m_ppOverlapTextures = 0;
	int	m_nObjects = 0;

	float m_fBillboardOffset[MONSTER_NUM + LOBBY_MAX_CLIENT + MAX_MINION] =
	{ 
		4.5f, 2.5f, 2.2f, 2.0f, 2.0f, 1.9f, 2.5f, 1.9f, 2.5f,
		2.6f, 2.6f, 2.6f, 2.6f,
		1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f
	};

private:
	CGameObject** monsters = NULL;
	CGameObject** minions = NULL;
	CGameObject** otherclients = NULL;
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CDebugNormalShader : public CStandardShader
{
public:
	CDebugNormalShader();
	virtual ~CDebugNormalShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);

};

class CDebugDiffuseShader : public CStandardShader
{
public:
	CDebugDiffuseShader();
	virtual ~CDebugDiffuseShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);

};

class CDebugTextureShader : public CStandardShader
{
public:
	CDebugTextureShader();
	virtual ~CDebugTextureShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);

};

class CDebugLightingShader : public CStandardShader
{
public:
	CDebugLightingShader();
	virtual ~CDebugLightingShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState(int nPipelineState = 0);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CBoundingBoxShader : public CStandardShader
{
public:
	CBoundingBoxShader();
	virtual ~CBoundingBoxShader();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);
	virtual void ReleaseObjects();

	virtual void ReleaseUploadBuffers();

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);
	void Update(CPlayer* player);

protected:
	CGameObject** m_ppObjects = 0;
	int								m_nObjects = 0;

	CMaterial* m_pMaterial = NULL;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CBlendApplyShader : public CSkinnedAnimationStandardShader
{

public:
	CBlendApplyShader();
	virtual ~CBlendApplyShader();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
};

class CBlendObjectShader : public CSkinnedAnimationObjectsShader
{

public:
	CBlendObjectShader();
	virtual ~CBlendObjectShader();
	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState = 0);
	virtual void PostRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap);
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
	virtual void BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CParticleShader : public CShader
{
public:
	CParticleShader();
	virtual ~CParticleShader();

	virtual D3D12_PRIMITIVE_TOPOLOGY_TYPE GetPrimitiveTopologyType(int nPipelineState);
	virtual UINT GetNumRenderTargets(int nPipelineState);
	virtual DXGI_FORMAT GetRTVFormat(int nPipelineState, int nRenderTarget);
	virtual DXGI_FORMAT GetDSVFormat(int nPipelineState);

	virtual D3D12_SHADER_BYTECODE CreateVertexShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreateGeometryShader(int nPipelineState = 0);
	virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout();
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState(int nPipelineState = 0);
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	virtual void CreateShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState);
	void CreateParticleShader(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, int nPipelineState);
};


class CBlendSkillShader : public CStandardShader
{

public:
	CBlendSkillShader();
	virtual ~CBlendSkillShader();

	virtual D3D12_BLEND_DESC CreateBlendState();
	//virtual D3D12_SHADER_BYTECODE CreatePixelShader(int nPipelineState = 0);
};