//------------------------------------------------------- ----------------------
// File: Object.h
//-----------------------------------------------------------------------------

#pragma once

#include "Mesh.h"
#include "Camera.h"


class CShader;
class CStandardShader;
class ModelPartSelection;
class SharedDdsTexture;
class ObjectConstantArena;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05
#define RESOURCE_TEXTURE1D			0x06
#define RESOURCE_STRUCTURED_BUFFER	0x07

constexpr std::array<int, 10> MOVE_ANIM = { 7, 8, 11, 22, 24, 25, 26, 43, 58, 59 };

struct Player_Info
{
	int MaxHp = 0;
	int CurHp = MaxHp;
	int MaxMp = 0;
	int CurMp = MaxMp;
	int Critical = 0;
	int Ad = 0;
	int Ap = 0;
	int Ad_Defence = 0;
	int Ap_Defence = 0;
	int Patience = 0;
	float Speed = 1;
};

struct PlayerStatus
{
	SKILL_BUFF skillBuff = SKILL_BUFF::NONE;
};

struct OBJECT_INFO
{
	XMFLOAT4X4							m_xmf4x4World;
	UINT									m_nObjectID;
	float								m_nObjectDissolveState;
};

struct MATERIAL_INFO
{
	XMFLOAT4						m_xmf4AmbientColor;
	XMFLOAT4						m_xmf4AlbedoColor;
	XMFLOAT4						m_xmf4SpecularColor;
	XMFLOAT4						m_xmf4EmissiveColor;
	
	UINT							m_nType;
};

class CTexture
{
public:
	CTexture(int nTextureResources, UINT nResourceType, int nSamplers, int nRootParameters);
	CTexture(const CTexture& rhs);
	virtual ~CTexture();

private:
	int								m_nReferences = 0;

	UINT							m_nTextureType;

	int								m_nTextures = 0;
	ID3D12Resource** m_ppd3dTextures = NULL;
	ID3D12Resource** m_ppd3dTextureUploadBuffers = nullptr;
	// 모델 DDS 슬롯의 raw resource는 이 소유자의 별칭이다. 직접 Release하지 않는다.
	std::vector<std::shared_ptr<SharedDdsTexture>> m_sharedDds;
	XMFLOAT4 m_uiUvTransform{1, 1, 0, 0};

	UINT* m_pnResourceTypes = NULL;

	DXGI_FORMAT* m_pdxgiBufferFormats = NULL;
	int* m_pnBufferElements = NULL;
	int* m_pnBufferStrides = NULL;
	int								m_nRootParameters = 0;
	std::shared_ptr<UINT[]> m_sharepnRootParameterIndices = NULL;
	std::shared_ptr<D3D12_GPU_DESCRIPTOR_HANDLE[]> m_sharepd3dSrvGpuDescriptorHandles = nullptr;

	int								m_nSamplers = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE* m_pd3dSamplerGpuDescriptorHandles = NULL;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

	void SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle);

	void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex);
	void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	void ReleaseShaderVariables();

	void LoadTextureFromDDSFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, const wchar_t* pszFileName, UINT nResourceType, UINT nIndex);
	void LoadSharedTextureFromDDSFile(ID3D12Device* device, ID3D12GraphicsCommandList* commands, const wchar_t* path, UINT resourceType, UINT index);
	void LoadBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, UINT nIndex);
	void CreateBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, UINT nIndex);
	ID3D12Resource* CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, UINT nIndex, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nElements, UINT nMipLevels, DXGI_FORMAT dxgiFormat, D3D12_RESOURCE_FLAGS d3dResourceFlags, D3D12_RESOURCE_STATES d3dResourceStates, D3D12_CLEAR_VALUE* pd3dClearValue);

	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex);
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle);

	int GetRootParameters() { return(m_nRootParameters); }
	UINT GetRootParameterIndex(int index) const { return m_sharepnRootParameterIndices[index]; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int index) const { return m_sharepd3dSrvGpuDescriptorHandles[index]; }
	int GetTextures() { return(m_nTextures); }
	ID3D12Resource* GetResource(int nIndex) { return(m_ppd3dTextures[nIndex]); }
	XMFLOAT4 GetUiUvTransform() const { return m_uiUvTransform; }
	ID3D12Resource* GetUploadResource(int index) const { return m_ppd3dTextureUploadBuffers[index]; }
	const std::shared_ptr<SharedDdsTexture>& GetSharedDds(int index) const { return m_sharedDds[index]; }

	UINT GetTextureType() { return(m_nTextureType); }
	UINT GetTextureType(int nIndex) { return(m_pnResourceTypes[nIndex]); }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_pnBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);

	void ReleaseUploadBuffers();
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
#define MATERIAL_ALBEDO_MAP				0x01
#define MATERIAL_SPECULAR_MAP			0x02
#define MATERIAL_NORMAL_MAP				0x04
#define MATERIAL_METALLIC_MAP			0x08
#define MATERIAL_EMISSION_MAP			0x10
#define MATERIAL_DETAIL_ALBEDO_MAP		0x20
#define MATERIAL_DETAIL_NORMAL_MAP		0x40

class CGameObject;

class CMaterial
{
public:
	CMaterial(int nTextures);
	virtual ~CMaterial();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

public:
	CShader							*m_pShader = NULL;

	XMFLOAT4						m_xmf4AlbedoColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	XMFLOAT4						m_xmf4EmissiveColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4SpecularColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4						m_xmf4AmbientColor = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);

	void SetShader(CShader *pShader);
	void SetMaterialType(UINT nType) { m_nType |= nType; }
	void SetTexture(CTexture *pTexture, UINT nTexture = 0);

	virtual void CreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();

	virtual void ReleaseUploadBuffers();

public:
	UINT							m_nType = 0x00;

	float							m_fGlossiness = 0.0f;
	float							m_fSmoothness = 0.0f;
	float							m_fSpecularHighlight = 0.0f;
	float							m_fMetallic = 0.0f;
	float							m_fGlossyReflection = 0.0f;

public:
	int 							m_nTextures = 0;
	_TCHAR							(*m_ppstrTextureNames)[64] = NULL;
	CTexture						**m_ppTextures = NULL; //0:Albedo, 1:Specular, 2:Metallic, 3:Normal, 4:Emission, 5:DetailAlbedo, 6:DetailNormal

	ID3D12Resource* m_pd3dcbMaterial = NULL;
	MATERIAL_INFO* m_pcbMappedMaterial = NULL;

	void LoadTextureFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, UINT nType, UINT nRootParameter, _TCHAR *pwstrTextureName, CTexture **ppTexture, CGameObject *pParent, FILE *pInFile, CShader *pShader);

public:
	static CShader					*m_pStandardShader;
	static CShader					*m_pSkinnedAnimationShader;

	static void PrepareShaders(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);

	void SetStandardShader() { CMaterial::SetShader(m_pStandardShader); }
	void SetSkinnedAnimationShader() { CMaterial::SetShader(m_pSkinnedAnimationShader); }
};

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
struct CALLBACKKEY
{
   float  							m_fTime = 0.0f;
   void  							*m_pCallbackData = NULL;
};

#define _WITH_ANIMATION_INTERPOLATION

class CAnimationCallbackHandler
{
public:
	CAnimationCallbackHandler() { }
	~CAnimationCallbackHandler() { }

public:
   virtual void HandleCallback(void *pCallbackData, float fTrackPosition) { }
};

//#define _WITH_ANIMATION_SRT

class CAnimationSet
{
public:
	CAnimationSet(float fLength, int nFramesPerSecond, int nKeyFrameTransforms, int nSkinningBones, char *pstrName);
	~CAnimationSet();

public:
	char							m_pstrAnimationSetName[64];

	float							m_fLength = 0.0f;
	int								m_nFramesPerSecond = 0; //m_fTicksPerSecond

	int								m_nKeyFrames = 0;
	float							*m_pfKeyFrameTimes = NULL;
	XMFLOAT4X4						**m_ppxmf4x4KeyFrameTransforms = NULL;

#ifdef _WITH_ANIMATION_SRT
	int								m_nKeyFrameScales = 0;
	float							*m_pfKeyFrameScaleTimes = NULL;
	XMFLOAT3						**m_ppxmf3KeyFrameScales = NULL;
	int								m_nKeyFrameRotations = 0;
	float							*m_pfKeyFrameRotationTimes = NULL;
	XMFLOAT4						**m_ppxmf4KeyFrameRotations = NULL;
	int								m_nKeyFrameTranslations = 0;
	float							*m_pfKeyFrameTranslationTimes = NULL;
	XMFLOAT3						**m_ppxmf3KeyFrameTranslations = NULL;
#endif

public:
	XMFLOAT4X4 GetSRT(int nBone, float fPosition);
};

class CAnimationSets
{
public:
	CAnimationSets(int nAnimationSets);
	~CAnimationSets();

private:
	int								m_nReferences = 0;

public:
	void AddRef() { m_nReferences++; }
	void Release() { if (--m_nReferences <= 0) delete this; }

public:
	int								m_nAnimationSets = 0;
	CAnimationSet					**m_pAnimationSets = NULL;

	int								m_nAnimatedBoneFrames = 0; 
	CGameObject						**m_ppAnimatedBoneFrameCaches = NULL; //[m_nAnimatedBoneFrames]
};

class CAnimationTrack
{
public:
	CAnimationTrack() { }
	~CAnimationTrack();

public:
    BOOL 							m_bEnable = true;
    float 							m_fSpeed = 1.0f;
    float 							m_fPosition = -ANIMATION_CALLBACK_EPSILON;
	float 							m_fWeight = 1.0f;
	float							m_fBlendingW = 0.f;

	int 							m_nAnimationSet = 0;

	int 							m_nType = ANIMATION_TYPE_LOOP; //Once, Loop, PingPong

	bool							m_bContinuousAni = false;

	int 							m_nCallbackKeys = 0;
	CALLBACKKEY*					m_pCallbackKeys = NULL;

	CAnimationCallbackHandler*		m_pAnimationCallbackHandler = NULL;

	//add Loop
	float							m_fAnimLoopTime = 0;
	float							m_fAnimLoopEndTime = 0;
	bool							m_bLoop = false;
	bool							m_bOnOff = false; 
	bool							m_bOnOffToggle = false; //true : on, false : off 

	int								m_iAnimLoopCount = 0;
	int								m_iAnimLoopEndCount = 0;

public:
	void SetAnimationSet(int nAnimationSet) { m_nAnimationSet = nAnimationSet; }

	void SetEnable(bool bEnable) { m_bEnable = bEnable; }
	void SetSpeed(float fSpeed) { m_fSpeed = fSpeed; }
	void SetWeight(float fWeight) { m_fWeight = fWeight; }
	void SetBlendingWeight(float fWeight) { m_fBlendingW = fWeight; }
	void SetContinuousAni(bool ContinuousAni) { m_bContinuousAni = ContinuousAni; }

	void SetPosition(float fPosition) { m_fPosition = fPosition; }
	float UpdatePosition(float fTrackPosition, float fTrackElapsedTime, float fAnimationLength);

	void SetCallbackKeys(int nCallbackKeys);
	void SetCallbackKey(int nKeyIndex, float fTime, void* pData);
	void SetAnimationCallbackHandler(CAnimationCallbackHandler* pCallbackHandler);

	void HandleCallback();
};

class CLoadedModelInfo
{
public:
	CLoadedModelInfo() { }
	~CLoadedModelInfo();

    CGameObject						*m_pModelRootObject = NULL;

	int 							m_nSkinnedMeshes = 0;
	CSkinnedMesh					**m_ppSkinnedMeshes = NULL; //[SkinnedMeshes], Skinned Mesh Cache

	CAnimationSets					*m_pAnimationSets = NULL;

public:
	void PrepareSkinning();
};

class CAnimationController 
{
public:
	CAnimationController(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, int nAnimationTracks, CLoadedModelInfo *pModel);
	~CAnimationController();

public:
    float 							m_fTime = 0.0f;

    int 							m_nAnimationTracks = 0;
    CAnimationTrack 				*m_pAnimationTracks = NULL;

	CAnimationSets					*m_pAnimationSets = NULL;

	int 							m_nSkinnedMeshes = 0;
	CSkinnedMesh					**m_ppSkinnedMeshes = NULL; //[SkinnedMeshes], Skinned Mesh Cache

	ID3D12Resource					**m_ppd3dcbSkinningBoneTransforms = NULL; //[SkinnedMeshes]
	XMFLOAT4X4						**m_ppcbxmf4x4MappedSkinningBoneTransforms = NULL; //[SkinnedMeshes]

public:
	void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);

	void SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet);
	int GetTrackAnimationSet(int nAnimationTrack) { return m_pAnimationTracks[nAnimationTrack].m_nAnimationSet; }

	void SetTrackEnable(int nAnimationTrack, bool bEnable);
	void SetTrackPosition(int nAnimationTrack, float fPosition);
	void SetTrackSpeed(int nAnimationTrack, float fSpeed);
	void SetTrackWeight(int nAnimationTrack, float fWeight);
	void SetTrackBlendingWeight(int nAnimationTrack, float fWeight);
	void SetTrackContinuousAni(int nAnimationTrack, bool bContinuousAni);
	bool GetTrackContinuousAni(int nAnimationTrack) { return  m_pAnimationTracks[nAnimationTrack].m_bContinuousAni; }
	void SetDetailSkillAnim_Hero(int nAnimationTrack, int iSkillNum, int iAnimNum);
	void SetDetailSkillAnim_Boss(int nAnimationTrack, int iSkillNum);

	void SetCallbackKeys(int nAnimationTrack, int nCallbackKeys);
	void SetCallbackKey(int nAnimationTrack, int nKeyIndex, float fTime, void *pData);
	
	void SetAnimationCallbackHandler(int nAnimationTrack, CAnimationCallbackHandler *pCallbackHandler);

	void AdvanceTime(float fElapsedTime, CGameObject *pRootGameObject);
	void CheckingAniChage();

	bool IsAnimationFinished(int nAnimationTrack) const;

public:
	CGameObject*					m_pModelRootObject = NULL;
	
	bool							m_bBlend = false;
	int								m_iPreTrackNum = 0;
	int								m_iPostTrackNum = 0;
	float							m_fInterpolateW = 0.f;
	float							m_fProgressTime = 0.f;

	bool							m_bAniChange = false;	
	int								m_iProgressAnimationTrack = 0;

	
	virtual void OnRootMotion(CGameObject* pRootGameObject) { }
	virtual void OnAnimationIK(CGameObject* pRootGameObject) { }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

class CGameObject
{
private:
	int								m_nReferences = 0;

public:
	void AddRef();
	int Release();

public:
	CGameObject();
	CGameObject(int nMaterials);
    virtual ~CGameObject();

public:
	char							m_pstrFrameName[64] = { '\0'};
	bool							m_bIsRender = true;
	bool m_bLoadAnimationFrame = true;

	CMesh							*m_pMesh = NULL;
	std::shared_ptr<CMesh> m_sharedMesh;

	int								m_nMaterials = 0;
	CMaterial						**m_ppMaterials = NULL;

	XMFLOAT4X4						m_xmf4x4ToParent;
	XMFLOAT4X4						m_xmf4x4World;

	CGameObject 					*m_pParent = NULL;
	CGameObject 					*m_pChild = NULL;
	CGameObject 					*m_pSibling = NULL;

	CAnimationController*			m_pSkinnedAnimationController = NULL;

	UINT m_nObjectID = 0;

	//curling
	float m_fMaxRadius = 0;
	OBJ_TYPE m_eObjType = OBJ_TYPE::OBJECT;
	float m_fDistanceCamera = 0;

	//dissolve
	float m_nObjectDissolveState = 0;
	float m_fDissolveTime = 0.f;

	vector<ID3D12Resource*> m_pd3dcbVecObject;
	vector<OBJECT_INFO*> m_pcbMappedVecObjects;
	vector<std::shared_ptr<ObjectConstantArena>> m_objectConstantOwners;
	int iTotalShareNum = 0;
	int iMyShareNum = 0;

	ANI_ON_SERVER		m_Ani = ANI_ON_SERVER::NONE;

	static const char* FrameNames[];

	void SetMesh(CMesh *pMesh);
	void SetSharedMesh(const std::shared_ptr<CMesh>& mesh);
	void SetShader(CShader *pShader);
	void SetShader(int nMaterial, CShader *pShader);
	void SetRootShader(CShader *pShader);
	void SetMaterial(int nMaterial, CMaterial *pMaterial);

	void SetChild(CGameObject *pChild, bool bReferenceUpdate=false);
	void RemoveChild(CGameObject* pChild);

	virtual void BuildMaterials(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList) { }

	virtual void OnPrepareAnimate() { }
	virtual void Animate(float fTimeElapsed);

	virtual void OnPrepareRender() { }
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL, int SharedNum = 0,int nPipelineState = 0);
	virtual void PureRender(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera=NULL, int nPipelineState = 0);

	virtual void OnLateUpdate() { }

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void AllCreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);
	
	virtual void ReleaseShaderVariables();

	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, XMFLOAT4X4 *pxmf4x4World, int ShardNum = 0);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList *pd3dCommandList, CMaterial *pMaterial);
	virtual void UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, bool dissovle);

	virtual void ReleaseUploadBuffers();
	virtual float GetHpPercentage() const { return 0.f; }
	virtual float GetMpPercentage() const { return 0.f; }

	XMFLOAT3 GetPosition();
	XMFLOAT3 GetLook();
	XMFLOAT3 GetUp();
	XMFLOAT3 GetRight();
	XMFLOAT3 GetScale();
	XMFLOAT2 GetScreenPosition();

	XMFLOAT3 GetToParentPosition();
	void Move(XMFLOAT3 xmf3Offset);

	void SetPosition(float x, float y, float z);
	void SetPosition(XMFLOAT3 xmf3Position);
	virtual void SetScreenPosition(XMFLOAT2 xmf2Position);
	void SetScale(float x, float y, float z);

	void SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3&& xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f));

	void Rotate(float fPitch = 10.0f, float fYaw = 10.0f, float fRoll = 10.0f);
	void Rotate(XMFLOAT3 *pxmf3Axis, float fAngle);
	void Rotate(XMFLOAT4 *pxmf4Quaternion);

	CGameObject *GetParent() { return(m_pParent); }
	void UpdateTransform(XMFLOAT4X4 *pxmf4x4Parent=NULL);
	CGameObject *FindFrame(const char *pstrFrameName);

	CTexture *FindReplicatedTexture(_TCHAR *pstrTextureName);

	UINT GetMeshType() { return((m_pMesh) ? m_pMesh->GetType() : 0x00); }

public:
	CSkinnedMesh *FindSkinnedMesh(char *pstrSkinnedMeshName);
	void FindAndSetSkinnedMesh(CSkinnedMesh **ppSkinnedMeshes, int *pnSkinnedMesh);

	void SetTrackAnimationSet(int nAnimationTrack, int nAnimationSet);
	void SetTrackAnimationPosition(int nAnimationTrack, float fPosition);

	void LoadMaterialsFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, CGameObject *pParent, FILE *pInFile, CShader *pShader);

	static void LoadAnimationFromFile(FILE *pInFile, CLoadedModelInfo *pLoadedModel);
	static CGameObject *LoadFrameHierarchyFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CGameObject *pParent, FILE *pInFile, CShader *pShader, int *pnSkinnedMeshes, const ModelPartSelection* selection = nullptr);

	static CLoadedModelInfo *LoadGeometryAndAnimationFromFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, const char *pstrFileName, CShader *pShader, const ModelPartSelection* selection = nullptr);

	static void PrintFrameInfo(CGameObject *pGameObject, CGameObject *pParent);	

	void DrawOff();
	void DrawOn();

	void SetObjectID(UINT id);
	UINT GetObjectID() { return m_nObjectID; }

	void SetObjectType(OBJ_TYPE eType);
	void SetDissolveState(float fAmount);
	void SetAddDissolveState(float fAmount);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CSkyBox : public CGameObject
{
public:
	CSkyBox(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature);
	virtual ~CSkyBox();

	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL, int SharedNum = 0, int nPipelineState = 0);
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CMap : public CGameObject
{
public:
	CMap(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks);
	virtual ~CMap();
};

class CModularModel : public CGameObject
{
public:
	CModularModel(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks);
	virtual ~CModularModel();

protected:
	ModelCustomize m_CustomizeInfo;
};

class CBBObject : public CGameObject
{
public:
	CBBObject(int nMeshes = 1);
	virtual ~CBBObject();
public:
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, int SharedNum = 0, int nPipelineState = 0);
};

class CPlayerObject : public CGameObject
{
public:
	CPlayerObject() {};
	virtual ~CPlayerObject() {};
	
	virtual float GetHpPercentage() const { return static_cast<float>(m_Info.CurHp) / m_Info.MaxHp; }
	virtual float GetMpPercentage() const { return static_cast<float>(m_Info.CurMp) / m_Info.MaxMp; }

	void Customize(ModelCustomize customization);
	void SetWeapon(JOB playerJob);
	void ModifyModel();
	void ChangeSex();
	ModelCustomize GetCustomizeInfo() { return m_CustomizeInfo; }
	
	void SetPlayerInfo(Player_Info info) { m_Info = info; }
	const Player_Info& GetPlayerInfo() const { return m_Info; }
	const PlayerStatus& GetStatus() const { return m_status; }
	void SetSkillBuff(SKILL_BUFF buff) { m_status.skillBuff = buff; }

protected:
	PlayerStatus m_status;
	Player_Info m_Info;
	ModelCustomize m_CustomizeInfo;

	set<int> m_moveAnimNum;
};

class COtherClientPlayer : public CPlayerObject
{
public:
	COtherClientPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int ClientNum);
	virtual ~COtherClientPlayer();

	void Update(int id);

	int GetAnimation() { return m_animation; }
	void SetReadyAnim(int AniNum);
	void SetIdleAnim();
	void SetAnimation(int AniNum, int preTrackNum, int postTrackNum);

private:
	void SetLook(float x, float y, float z);
	void SetRight(float x, float y, float z);
	void UseSkill(SKILLKIND nSkillNum, int id);
	
	

private:
	int m_animation = 0;
};

class CNpc : public CGameObject
{
public:
	CNpc();
	CNpc(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int type);
	virtual ~CNpc();

	virtual void Update(int id);

	void SetAnimation(int anim);
	int GetAnimation() const { return m_animation; }

	void SetMaxHp(int hp) { m_maxHp = hp; }
	void SetCurHp(int hp) { m_curHp = hp; }
	int GetMaxHp() const { return m_maxHp; }
	int GetCurHp() const { return m_curHp; }
	virtual float GetHpPercentage() const { return static_cast<float>(m_curHp) / m_maxHp; }

protected:
	void SetLook(float x, float y, float z);
	void SetRight(float x, float y, float z);

	int m_animation = 0;

	int m_maxHp = 0;
	int m_curHp = 0;
};

class CLobbyNpc : public CNpc
{
public:
	CLobbyNpc() {}
	CLobbyNpc(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CLobbyNpc();

	void SetTalkCallback(const function<void()>& callback) { onTalkCallback = callback; }

	CLobbyNpc* CanTalk(XMFLOAT3 pos);
	void Talk();
protected:
	float m_cognise = 5.0f;
	function<void()> onTalkCallback = nullptr;
};

class CMonster : public CNpc
{
public:
	CMonster() {}
	CMonster(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CMonster();

	virtual void Update(int id);

	int GetAttackAnim() const { return m_attackAnim; }
	int GetWalkAnim() const { return m_walkAnim; }
	int GetRunAnim() const { return m_runAnim; }
	int GetDeathAnim() const { return m_deathAnim; }
	const wstring& GetAttackSoundString() const { return m_attackSoundString; }
	const wstring& GetDeathSoundString() const { return m_deathSoundString; }

protected:
	int m_attackAnim = 0;
	int m_walkAnim = 0;
	int m_runAnim = 0;
	int m_deathAnim = 0;
	wstring m_attackSoundString = L"";
	wstring m_deathSoundString = L"";
};

class CUniqueRed : public CMonster
{
public:
	CUniqueRed(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CUniqueRed() {}
};

class CRareGreen : public CMonster
{
public:
	CRareGreen(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CRareGreen() {}
};

class CRareGolem : public CMonster
{
public:
	CRareGolem(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CRareGolem() {}
};

class CNormalBear : public CMonster
{
public:
	CNormalBear(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CNormalBear() {}
};

class CNormalMinotaur : public CMonster
{
public:
	CNormalMinotaur(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CNormalMinotaur() {}
};

class CNormalChest : public CMonster
{
public:
	CNormalChest(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CNormalChest() {}
};

class CNormalBeholder : public CMonster
{
public:
	CNormalBeholder(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CNormalBeholder() {}
};

class CMinion : public CNpc
{
public:
	CMinion(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int type);
	virtual ~CMinion();

	virtual void Update(int id);
};

class CTowerAttack : public CGameObject
{
public:
	CTowerAttack(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel);
	virtual ~CTowerAttack();

	void SetShow(bool show) { m_show = show; }
	bool GetShow() const { return m_show; }

	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL, int SharedNum = 0, int nPipelineState = 0) {};
private:
	bool m_show = false;
};

class CSkillObject : public CGameObject
{
public:
	CSkillObject() {}
	CSkillObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel);
	virtual ~CSkillObject() {}

	void Update(const XMFLOAT3& pos, const XMFLOAT3& look);

protected:
	void SetScaleValue(float x, float y, float z) { m_scaleValue = XMFLOAT3(x, y, z); }

protected:
	XMFLOAT3 m_scaleValue;
};

class CBlendObject : public CGameObject
{
public:
	CBlendObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, int nAnimationTracks);
	virtual ~CBlendObject();
};

class CUIObject : public CGameObject
{
public:
	CUIObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CTexture* texture, XMFLOAT2 meshRectSize, XMFLOAT2 screenPos, TEXTURETYPE type, float val, XMFLOAT2 uvOffset);
	virtual ~CUIObject();

	function<void()> onClickCallback = nullptr;
	function<void()> onHoverCallback = nullptr;
	function<void()> onHoverEndCallback = nullptr;
	function<void()> onReleaseCallback = nullptr;

	void SetOnClickCallback(const function<void()>& callback) { onClickCallback = callback; }
	void SetOnHoverCallback(const function<void()>& callback) { onHoverCallback = callback; }
	void SetOnHoverEndCallback(const function<void()>& callback) { onHoverEndCallback = callback; }
	void SetOnReleaseCallback(const function<void()>& callback) { onReleaseCallback = callback; }

	void OnClick();
	void OnHover();
	void OnHoverEnd();
	CUIObject* OnRelease();

	CUIObject* OnMouseMoved(float mouseX, float mouseY);

	UIOBJECTSTATE GetOBJState() const { return objState; }

	void SetBasicButtonEvents(); // Set Events for Basic Button

	virtual void SetScreenPosition(XMFLOAT2 xmf2Position);
private:
	CollisionBox collisionBox;
	UIOBJECTSTATE objState = UIOBJECTSTATE::DEFAULT;
};


struct PARTICLE_INFO
{
	XMFLOAT4							m_xm4Color;
	XMFLOAT3							m_xmForwardVector;
	float								m_fSize;
	float								m_fLifeTime;
	int									m_iParticleNum;
	int									m_iTotalSpriteNum;
	int									m_iWidthSpriteNum;
	int									m_iCurrentSpriteNum;
	int									m_iboolLean;
};

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
class CParticleObject : public CGameObject
{
public:
	CParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CTexture* Texture, CTexture* RandowmValueTexture, CTexture* RandowmValueSphereTexture,CShader* pShader, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Velocity, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles, UINT nType, std::shared_ptr<ParticleBufferPool> pool, bool eager = false);
	virtual ~CParticleObject();

	CTexture* m_pRandowmValueTexture = NULL;
	CTexture* m_pRandowmValueOnSphereTexture = NULL;

	void ReleaseUploadBuffers();
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void AllCreateShaderVariables(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	virtual void OnPostRender();

	void ReleaseInactiveBuffers() { reinterpret_cast<CParticleMesh*>(m_pMesh)->ReleaseInactiveBuffers(); }
	bool GetShow() {return m_bShow;}
	bool GetUpdateRotate() {return m_bUpdateRotate;}
	bool GetUpdatePosition() {return m_bUpdatePosition;}
	const XMFLOAT3& GetForwardVector() { return m_xmForwardVector; }

	void SetForwardVector(XMFLOAT3 forword) { m_xmForwardVector = forword; }
	void SetColor(XMFLOAT4 color) { m_xm4Color = color; }
	void SetSize(float size) { m_fSize = size; }
	void SetNum(int Num) { m_iParticleNum = Num; }
	void SetLife(float life) { m_fTotalLifeTime = m_fLifeTime = life; }
	void SetTotalSpriteNum(int TotalNum) { m_iTotalSpriteNum = TotalNum; }
	void SetWidthSpriteNum(int WidthNum) { m_iWidthSpriteNum = WidthNum; }
	void SetCurrentSpriteNum(int CurrentNum) { m_iCurrentSpriteNum = CurrentNum; }
	void SetShow(bool show) 
	{ 
		m_bShow = show; 
		if (!m_bShow)
		{
			reinterpret_cast<CParticleMesh*>(m_pMesh)->m_bStart = true;
		}
	}
	void SetUpdateRotate(bool update) { m_bUpdateRotate = update; }
	void SetUpdatePosition(bool update) { m_bUpdatePosition = update; }
	void SetboolLean(int lean) { m_iboolLean = lean; }
	void SetInfinity(bool infinity) {m_bInfinity = infinity;}

	void SettingDetail(PARTICLE_SITUATION situation, int num);
	void SettingDetail(SKILL_TYPE type, int num);

	void AnimateSprite(float time);
	bool AnimateLifeTime(float time);

private:
	ID3D12Resource* m_pd3dcbParticle = NULL;
	PARTICLE_INFO* m_pcbMappedParticle = NULL;
	XMFLOAT3 m_xmForwardVector = XMFLOAT3(1.0, 0, 0);
	XMFLOAT4							m_xm4Color = XMFLOAT4(0.1f, 1.0f, 1.0f, 1.0f);
	float								m_fSize = 1.0f;
	float								m_fLifeTime = 5.f;
	float								m_fTotalLifeTime = 5.f;
	int									m_iParticleNum = 60;
	int									m_iTotalSpriteNum = 0;
	int									m_iWidthSpriteNum = 0;
	int									m_iCurrentSpriteNum = 0;
	int									m_iboolLean = 0;
	float								m_Time = 0.f;
	bool								m_bShow = false;
	bool								m_bUpdateRotate = false;
	bool								m_bUpdatePosition = false;
	bool								m_bInfinity = false;
	
};
