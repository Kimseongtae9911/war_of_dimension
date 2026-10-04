#pragma once

#define DIR_FORWARD				0x01
#define DIR_BACKWARD			0x02
#define DIR_LEFT				0x04
#define DIR_RIGHT				0x08
#define DIR_UP					0x10
#define DIR_DOWN				0x20

#define VK_A					0x21
#define KEY_T					0x54

#include "Object.h"
#include "Camera.h"

class CPlayer : public CPlayerObject
{
protected:
	XMFLOAT3					m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	XMFLOAT3					m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	XMFLOAT3					m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	XMFLOAT3					m_xmf3Scale = XMFLOAT3(1.0f, 1.0f, 1.0f);

	float           			m_fPitch = 0.0f;
	float           			m_fYaw = 0.0f;
	float           			m_fRoll = 0.0f;

	XMFLOAT3					m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	XMFLOAT3     				m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	float           			m_fMaxVelocityXZ = 0.0f;
	float           			m_fMaxVelocityY = 0.0f;
	float           			m_fFriction = 0.0f;

	CCamera						*m_pCamera = NULL;
	int							m_nNowAnimation = 0;
	

	Additional_Stats			m_nAdditionalStats;

	// Added By Server Programmer
	char m_dir = 0x00;
	int m_id = -1;
	bool m_usingSkill = false;

public:
	CPlayer();
	virtual ~CPlayer();

	XMFLOAT3 GetPosition() { return(m_xmf3Position); }
	XMFLOAT3 GetLookVector() { return(m_xmf3Look); }
	XMFLOAT3 GetUpVector() { return(m_xmf3Up); }
	XMFLOAT3 GetRightVector() { return(m_xmf3Right); }

	void SetFriction(float fFriction) { m_fFriction = fFriction; }
	void SetGravity(const XMFLOAT3& xmf3Gravity) { m_xmf3Gravity = xmf3Gravity; }
	void SetMaxVelocityXZ(float fMaxVelocity) { m_fMaxVelocityXZ = fMaxVelocity; }
	void SetMaxVelocityY(float fMaxVelocity) { m_fMaxVelocityY = fMaxVelocity; }
	void SetVelocity(const XMFLOAT3& xmf3Velocity) { m_xmf3Velocity = xmf3Velocity; }
	void SetPosition(const XMFLOAT3& xmf3Position) { Move(XMFLOAT3(xmf3Position.x - m_xmf3Position.x, xmf3Position.y - m_xmf3Position.y, xmf3Position.z - m_xmf3Position.z), false); }

	void SetScale(XMFLOAT3& xmf3Scale) { m_xmf3Scale = xmf3Scale; }
	void SetScale(XMFLOAT3&& xmf3Scale) { m_xmf3Scale = xmf3Scale; }

	void SetDirection(char dir) { m_dir = dir; }

	const XMFLOAT3& GetVelocity() const { return(m_xmf3Velocity); }
	float GetYaw() const { return(m_fYaw); }
	float GetPitch() const { return(m_fPitch); }
	float GetRoll() const { return(m_fRoll); }

	CCamera *GetCamera() { return(m_pCamera); }
	void SetCamera(CCamera *pCamera) { m_pCamera = pCamera; }

	virtual void Move(DWORD nDirection, float fDistance, bool bVelocity = false);
	void Move(const XMFLOAT3& xmf3Shift, bool bVelocity = false);	
	void Rotate(float x, float y, float z);

	virtual void Update(float fTimeElapsed);ModelCustomize m_CustomizeInfo;

	virtual void OnCameraUpdateCallback(float fTimeElapsed) { }	

	virtual void CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	virtual void ReleaseShaderVariables();
	virtual void UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList);

	CCamera *OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode);

	virtual CCamera *ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed) { return(NULL); }
	virtual void OnPrepareRender();
	virtual void Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera = NULL, int pipelineState = 0);

	void SetNowAnimation(int animation) { m_nNowAnimation = animation; }
	int GetNowAnimation() { return m_nNowAnimation; }

	// Added by Server Programmer
	const char GetDir() const { return m_dir; }
	const int GetID() const { return m_id; }	

	float GetHpPercentage() const override { return static_cast<float>(m_Info.CurHp) / static_cast<float>(m_Info.MaxHp); }
	float GetMpPercentage() const override { return static_cast<float>(m_Info.CurMp) / static_cast<float>(m_Info.MaxMp); }

	Additional_Stats& GetAdditionalStats() { return m_nAdditionalStats; }
	void UpdateAttributeValue(int& attribute, int& point, int val);
	void AdjustStats(int statKind, int val);

	void SetUsingSkill(bool useskill) { m_usingSkill = useskill; }
};

class CSoundCallbackHandler : public CAnimationCallbackHandler
{
public:
	CSoundCallbackHandler() { }
	~CSoundCallbackHandler() { }

public:
	virtual void HandleCallback(void *pCallbackData, float fTrackPosition); 
};

class CGamePlayer : public CPlayer
{
public:
	CGamePlayer(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void *pContext=NULL);
	virtual ~CGamePlayer();

public:
	virtual CCamera *ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);

	virtual void OnPlayerUpdateCallback(float fTimeElapsed);

	virtual void Move(DWORD nDirection, float fDistance, bool bVelocity = false);

	virtual void Update(float fTimeElapsed);

	void SetReadyAnim(int ChangeJob);
	void SetDeathAnim();
	void SetIdleAnim();

	int GetSkillCoolTime(int skillNum) { return m_skillCoolTime[skillNum]; }

	bool CheckCoolTime(SKILLKIND skillNum);
	void LoadCoolTime();
	void UpdateRemaingTime();

	void SetGold(short gold) { m_gold = gold; }
	short GetGold() const { return m_gold; }
	void SetJumping(bool jumping) { m_jumping = jumping; }

	void UseSkillCheck(SKILLKIND nSkillNum);

	bool IsBagFull();
	void SetItem(ITEMKIND item);
	void UseItem(int index);
	array<ITEMKIND, 5> GetItemList() { return m_itemList; }

	void StatLevelUp(int index);
	Stat_Level GetStats() { return m_statLevels; }

private:
	void UseSkill(SKILLKIND nSkillNum);

	array<ITEMKIND, 5> m_itemList = { ITEMKIND::NONE, ITEMKIND::NONE, ITEMKIND::NONE, ITEMKIND::NONE, ITEMKIND::NONE };
	Stat_Level m_statLevels;
	XMFLOAT3 m_customizePos;
private:
	// Added by Server Programmer
	array<int, 5> m_skillCoolTime = {};	
	short m_gold = 0;
	bool m_jumping = false;
};

class CTitlePlayer : public CPlayer
{
public:
	CTitlePlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext = NULL);
	virtual ~CTitlePlayer();

public:
	virtual CCamera* ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);
};

class CReadyPlayer : public CGamePlayer
{
public:
	CReadyPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext = NULL);
	virtual ~CReadyPlayer();

public:
	virtual CCamera* ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed);
};