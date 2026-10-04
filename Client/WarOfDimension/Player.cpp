//-----------------------------------------------------------------------------
// File: CPlayer.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Player.h"
#include "Shader.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "Frustum.h"
#include "RenderManager.h"
#include "SoundManager.h"
#include "Shader.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// CPlayer

namespace BASIC_ANI
{
	// 전방 선언
	int Idle(JOB eJob);
	int WalkForward(JOB eJob);
	int WalkBack(JOB eJob);
	int WalkLeft(JOB eJob);
	int WalkRight(JOB eJob);
	int Death(JOB eJob);
	int Attack(JOB eJob);
}

namespace BOSS_OGRE_ANI
{
	int Idle();
	int WalkForward();
	int WalkBack();
	int WalkLeft();
	int WalkRight();
	int Attack();
	int Skill_AniNum(int skillNum);
	int Death();
}

namespace BOSS_PROGRAMMER_ANI
{
	int Idle();
	int Attack();
	int Skill_AniNum(int skillNum);
	int Death();
}

CPlayer::CPlayer()
{
	m_pCamera = NULL;

	m_xmf3Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Right = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_xmf3Up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	m_xmf3Look = XMFLOAT3(0.0f, 0.0f, 1.0f);

	m_xmf3Velocity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_xmf3Gravity = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_fMaxVelocityXZ = 0.0f;
	m_fMaxVelocityY = 0.0f;
	m_fFriction = 0.0f;

	m_fPitch = 0.0f;
	m_fRoll = 0.0f;
	m_fYaw = 0.0f;
}

CPlayer::~CPlayer()
{
	ReleaseShaderVariables();

	if (m_pCamera) delete m_pCamera;
}

void CPlayer::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pCamera) m_pCamera->CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CPlayer::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
}

void CPlayer::ReleaseShaderVariables()
{
	if (m_pCamera) m_pCamera->ReleaseShaderVariables();
}

void CPlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{
	if (CTextureShader::GetInstance()->IsShopping()) {
		CS_MOVE_PACKET* p = new CS_MOVE_PACKET;
		m_dir = 0;
		p->direction = m_dir;
		p->move_time = std::chrono::high_resolution_clock::now();
		p->size = sizeof(CS_MOVE_PACKET);
		p->type = CS_MOVE;
		NetworkManager::GetInstance()->SendPacket(p);
		CTextureShader::GetInstance()->AccelerateTextureSwitch(false);
		return;
	}

	char dir = 0x00;
	if (dwDirection)
	{
		if (dwDirection & DIR_FORWARD) {
			dir |= DIR_FORWARD;
		}
		if (dwDirection & DIR_BACKWARD) {
			dir |= DIR_BACKWARD;
		}
		if (dwDirection & DIR_RIGHT) {			
			dir |= DIR_RIGHT;
		}
		if (dwDirection & DIR_LEFT) {
			dir |= DIR_LEFT;
		}
		if (dwDirection & DIR_UP) {
			dir |= DIR_UP;
		}
		if (dwDirection & DIR_DOWN) {
			dir |= DIR_DOWN;
		}

		if (m_dir != dir) {
			CS_MOVE_PACKET* p = new CS_MOVE_PACKET;
			p->direction = dir;
			p->move_time = std::chrono::high_resolution_clock::now();
			p->size = sizeof(CS_MOVE_PACKET);
			p->type = CS_MOVE;
			NetworkManager::GetInstance()->SendPacket(p);
			m_dir = dir;
		}

	}
	else if (m_dir != 0 && dir == 0) {
		CS_MOVE_PACKET* p = new CS_MOVE_PACKET;
		p->direction = dir;
		p->move_time = std::chrono::high_resolution_clock::now();
		p->size = sizeof(CS_MOVE_PACKET);
		p->type = CS_MOVE;
		NetworkManager::GetInstance()->SendPacket(p);
		m_dir = dir;
		CTextureShader::GetInstance()->AccelerateTextureSwitch(false);
	}
}

void CPlayer::Move(const XMFLOAT3& xmf3Shift, bool bUpdateVelocity)
{
	if (bUpdateVelocity)
	{
		m_xmf3Velocity = Vector3::Add(m_xmf3Velocity, xmf3Shift);
	}
	else
	{
		m_xmf3Position = Vector3::Add(m_xmf3Position, xmf3Shift);
		m_pCamera->Move(xmf3Shift);
	}

}

void CPlayer::Rotate(float x, float y, float z)
{
	if (m_usingSkill || CTextureShader::GetInstance()->IsShopping()) {
		return;
	}

	if (m_status.skillBuff == SKILL_BUFF::STUN && SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
		cout << "Stun" << endl;
		return;
	}
	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	if ((nCurrentCameraMode == FIRST_PERSON_CAMERA) || (nCurrentCameraMode == THIRD_PERSON_CAMERA))
	{
		if (x != 0.0f)
		{
			m_fPitch += x;
			if (m_fPitch > +89.0f) { x -= (m_fPitch - 89.0f); m_fPitch = +89.0f; }
			if (m_fPitch < -89.0f) { x -= (m_fPitch + 89.0f); m_fPitch = -89.0f; }
		}
		if (y != 0.0f)
		{
			m_fYaw += y;
			if (m_fYaw > 360.0f) m_fYaw -= 360.0f;
			if (m_fYaw < 0.0f) m_fYaw += 360.0f;
		}
		if (z != 0.0f)
		{
			m_fRoll += z;
			if (m_fRoll > +20.0f) { z -= (m_fRoll - 20.0f); m_fRoll = +20.0f; }
			if (m_fRoll < -20.0f) { z -= (m_fRoll + 20.0f); m_fRoll = -20.0f; }
		}
		m_pCamera->Rotate(x, y, z);
		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}
	else if (nCurrentCameraMode == SPACESHIP_CAMERA)
	{
		m_pCamera->Rotate(x, y, z);
		if (x != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Right), XMConvertToRadians(x));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
		}
		if (y != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Up), XMConvertToRadians(y));
			m_xmf3Look = Vector3::TransformNormal(m_xmf3Look, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
		if (z != 0.0f)
		{
			XMMATRIX xmmtxRotate = XMMatrixRotationAxis(XMLoadFloat3(&m_xmf3Look), XMConvertToRadians(z));
			m_xmf3Up = Vector3::TransformNormal(m_xmf3Up, xmmtxRotate);
			m_xmf3Right = Vector3::TransformNormal(m_xmf3Right, xmmtxRotate);
		}
	}

	m_xmf3Look = Vector3::Normalize(m_xmf3Look);
	m_xmf3Right = Vector3::CrossProduct(m_xmf3Up, m_xmf3Look, true);
	m_xmf3Up = Vector3::CrossProduct(m_xmf3Look, m_xmf3Right, true);

	//cout << m_xmf3Look.x<<", " << m_xmf3Look.y << ", " << m_xmf3Look.z << endl;

	NetworkManager::GetInstance()->SendRotatePacket(m_xmf3Look, m_xmf3Right);
}

void CPlayer::Update(float fTimeElapsed)
{
	// Set Position
	SetPosition(XMFLOAT3(NetworkManager::GetInstance()->myInfo->x, NetworkManager::GetInstance()->myInfo->y, NetworkManager::GetInstance()->myInfo->z));

	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	XMFLOAT3 playerPos = GetPosition();
	playerPos.y += 2.0f;
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA || nCurrentCameraMode == READY_SCENE_CAMERA) {
		m_pCamera->Update(m_xmf3Position, fTimeElapsed);
	}
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA || nCurrentCameraMode == READY_SCENE_CAMERA) m_pCamera->SetLookAt(playerPos);
	m_pCamera->RegenerateViewMatrix();
	Frustum::GetInstance()->m_xmfCamera4x4View = m_pCamera->GetViewMatrix();
	Frustum::GetInstance()->m_xmf4x4CameraProjection = m_pCamera->GetProjectionMatrix();
	Frustum::GetInstance()->Update();

	//RenderManager::GetInstance()->SetCameraPosition(m_pCamera->GetPosition());
}

CCamera *CPlayer::OnChangeCamera(DWORD nNewCameraMode, DWORD nCurrentCameraMode)
{
	CCamera *pNewCamera = NULL;
	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			pNewCamera = new CFirstPersonCamera(m_pCamera);
			break;
		case THIRD_PERSON_CAMERA:
			pNewCamera = new CThirdPersonCamera(m_pCamera);
			break;
		case SPACESHIP_CAMERA:
			pNewCamera = new CSpaceShipCamera(m_pCamera);
			break;
		case READY_SCENE_CAMERA:
			pNewCamera = new CReadySceneCamera(m_pCamera);
			break;
	}
	if (nCurrentCameraMode == SPACESHIP_CAMERA)
	{
		m_xmf3Right = Vector3::Normalize(XMFLOAT3(m_xmf3Right.x, 0.0f, m_xmf3Right.z));
		m_xmf3Up = Vector3::Normalize(XMFLOAT3(0.0f, 1.0f, 0.0f));
		m_xmf3Look = Vector3::Normalize(XMFLOAT3(m_xmf3Look.x, 0.0f, m_xmf3Look.z));

		m_fPitch = 0.0f;
		m_fRoll = 0.0f;
		m_fYaw = Vector3::Angle(XMFLOAT3(0.0f, 0.0f, 1.0f), m_xmf3Look);
		if (m_xmf3Look.x < 0.0f) m_fYaw = -m_fYaw;
	}
	else if ((nNewCameraMode == SPACESHIP_CAMERA) && m_pCamera)
	{
		m_xmf3Right = m_pCamera->GetRightVector();
		m_xmf3Up = m_pCamera->GetUpVector();
		m_xmf3Look = m_pCamera->GetLookVector();
	}

	if (pNewCamera)
	{
		pNewCamera->SetMode(nNewCameraMode);
		pNewCamera->SetPlayer(this);
	}

	if (m_pCamera) delete m_pCamera;

	return(pNewCamera);
}

void CPlayer::OnPrepareRender()
{
	m_xmf4x4ToParent._11 = m_xmf3Right.x; m_xmf4x4ToParent._12 = m_xmf3Right.y; m_xmf4x4ToParent._13 = m_xmf3Right.z;
	m_xmf4x4ToParent._21 = m_xmf3Up.x; m_xmf4x4ToParent._22 = m_xmf3Up.y; m_xmf4x4ToParent._23 = m_xmf3Up.z;
	m_xmf4x4ToParent._31 = m_xmf3Look.x; m_xmf4x4ToParent._32 = m_xmf3Look.y; m_xmf4x4ToParent._33 = m_xmf3Look.z;
	m_xmf4x4ToParent._41 = m_xmf3Position.x; m_xmf4x4ToParent._42 = m_xmf3Position.y; m_xmf4x4ToParent._43 = m_xmf3Position.z;

	m_xmf4x4ToParent = Matrix4x4::Multiply(XMMatrixScaling(m_xmf3Scale.x, m_xmf3Scale.y, m_xmf3Scale.z), m_xmf4x4ToParent);
}

void CPlayer::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, int pipelineState)
{
	if (!NetworkManager::GetInstance()->myInfo->show)
		return;
	DWORD nCameraMode = (pCamera) ? pCamera->GetMode() : 0x00;
	if ((SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY || SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) && NetworkManager::GetInstance()->GetId() == 3) {
		
	}
	else if ((SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY || SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) && NetworkManager::GetInstance()->GetId() != 3) {		
		Customize(NetworkManager::GetInstance()->m_ArrayInGameClientsCustom[NetworkManager::GetInstance()->GetId()]);
		SetWeapon(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()]));
	}
	else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::TITLE) {

	}
	else {
#ifdef WITH_DATABASE
		Customize(NetworkManager::GetInstance()->m_ArrayOtherClientCustom[NetworkManager::GetInstance()->GetId()]);
#endif
	}
	if (nCameraMode == THIRD_PERSON_CAMERA || nCameraMode == READY_SCENE_CAMERA) CGameObject::Render(pd3dCommandList, pCamera, pipelineState);
}

void CPlayer::UpdateAttributeValue(int& attribute, int& point, int val)
{
	int newValue = attribute + (val > 0 ? 1 : -1);
	int newPoint = point + (val > 0 ? -1 : 1);

	if (newValue >= 0 && newValue <= 15 && newPoint >= 0 && newPoint <= 15)
	{
		attribute = newValue;
		point = newPoint;
	}
}

void CPlayer::AdjustStats(int statKind, int val)
{
	if (m_nAdditionalStats.point + (val > 0 ? -1 : 1) >= 0 && m_nAdditionalStats.point + (val > 0 ? -1 : 1) <= 15) {
		switch (statKind)
		{
		case 1: UpdateAttributeValue(m_nAdditionalStats.hp, m_nAdditionalStats.point, val); break;
		case 2: UpdateAttributeValue(m_nAdditionalStats.mp, m_nAdditionalStats.point, val); break;
		case 3: UpdateAttributeValue(m_nAdditionalStats.attack, m_nAdditionalStats.point, val); break;
		case 4: UpdateAttributeValue(m_nAdditionalStats.magic_attack, m_nAdditionalStats.point, val); break;
		case 5: UpdateAttributeValue(m_nAdditionalStats.defense, m_nAdditionalStats.point, val); break;
		case 6: UpdateAttributeValue(m_nAdditionalStats.magic_defense, m_nAdditionalStats.point, val); break;
		case 7: UpdateAttributeValue(m_nAdditionalStats.speed, m_nAdditionalStats.point, val); break;
		case 8: UpdateAttributeValue(m_nAdditionalStats.tenacity, m_nAdditionalStats.point, val); break;
		case 9: UpdateAttributeValue(m_nAdditionalStats.critical, m_nAdditionalStats.point, val); break;
		}
	}

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define _WITH_DEBUG_CALLBACK_DATA

void CSoundCallbackHandler::HandleCallback(void *pCallbackData, float fTrackPosition)
{
   _TCHAR *pWavName = (_TCHAR *)pCallbackData; 
#ifdef _WITH_DEBUG_CALLBACK_DATA
	TCHAR pstrDebug[256] = { 0 };
	_stprintf_s(pstrDebug, 256, _T("%s(%f)\n"), pWavName, fTrackPosition);
	OutputDebugString(pstrDebug);
#endif
#ifdef _WITH_SOUND_RESOURCE
   PlaySound(pWavName, ::ghAppInstance, SND_RESOURCE | SND_ASYNC);
#else
   PlaySound(pWavName, NULL, SND_FILENAME | SND_ASYNC);
#endif
}

CGamePlayer::CGamePlayer(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature *pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void *pContext)
{
	for (int anim : MOVE_ANIM) {
		m_moveAnimNum.insert(anim);
	}

	m_pCamera = ChangeCamera(THIRD_PERSON_CAMERA, 0.0f);
	CLoadedModelInfo* pAngrybotModel = nullptr;
	

	if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::INGAME)
	{
		if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY)
		{
			if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
			{
				SetChild(pModel->m_pModelRootObject, true);
			}
			else
			{
				pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Boss_Ogre.bin", NULL);
				SetChild(pAngrybotModel->m_pModelRootObject, true);
			}
		}
		else
		{
			SetChild(pModel->m_pModelRootObject, true);
		}

		

		if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY)
		{
			m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 5, pModel);
			m_pSkinnedAnimationController->SetTrackAnimationSet(0, 12);	//idle
			m_pSkinnedAnimationController->SetTrackAnimationSet(1, 13);	//127.0.0.1

			m_pSkinnedAnimationController->SetTrackAnimationSet(2, 14);	//
			m_pSkinnedAnimationController->SetTrackAnimationSet(3, 15);	//
			m_pSkinnedAnimationController->SetTrackAnimationSet(4, 16);	//walk

			for (int i = 0; i < 5; ++i)
				m_pSkinnedAnimationController->SetTrackEnable(i, false);
			for (int i = 1; i < 5; ++i)
				m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);

			//m_pSkinnedAnimationController->SetCallbackKeys(1, 2);
		}
		else if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY)
		{
			if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 4, pModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, BASIC_ANI::Idle(JOB::ARCHER));	//Fighter idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(1, BASIC_ANI::Idle(JOB::FIGHTER));	//Fighter idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(2, BASIC_ANI::Idle(JOB::SWORDMAN));	//Fighter idle
				m_pSkinnedAnimationController->SetTrackAnimationSet(3, BASIC_ANI::Idle(JOB::WIZARD));	//Fighter idle

				for(int i=0; i < 4; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
			}
			else
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 1, pAngrybotModel);
				m_pSkinnedAnimationController->SetTrackAnimationSet(0, 0);	//idle
				m_pSkinnedAnimationController->SetTrackEnable(0, false);
			}
		}
	}
	else
	{
		if(SceneManager::GetInstance()->GetOrder() == ORDER::BOSS)
			if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
			{
				pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Boss_Ogre.bin", NULL);
				SetChild(pAngrybotModel->m_pModelRootObject, true);
			}
			else if(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
			{
				pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/Boss_Programmer.bin", NULL);
				SetChild(pAngrybotModel->m_pModelRootObject, true);
			}
			else
				cout << "An error occurs when setting the player as boss because the job" << endl;
		else
		{
			pAngrybotModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, "Model/ModularModel.bin", NULL);
			SetChild(pAngrybotModel->m_pModelRootObject, true);
			
#ifdef WITH_DATABASE
			ModelCustomize temp = { 0, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
			0 , -1, 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 0 };
#else
			ModelCustomize temp = { 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
			1, 0, 1, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1 };
#endif

			Customize(temp);
			NetworkManager::GetInstance()->SendModelCustomizePacket(temp);
		}



		if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
		{
			int AniTrack = 15;

			m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, AniTrack, pAngrybotModel);
			m_pSkinnedAnimationController->SetTrackAnimationSet(0, BASIC_ANI::Idle(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//idle
			m_pSkinnedAnimationController->SetTrackAnimationSet(1, BASIC_ANI::WalkForward(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//
			m_pSkinnedAnimationController->SetTrackAnimationSet(2, BASIC_ANI::WalkBack(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//
			m_pSkinnedAnimationController->SetTrackAnimationSet(3, BASIC_ANI::WalkLeft(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//
			m_pSkinnedAnimationController->SetTrackAnimationSet(4, BASIC_ANI::WalkRight(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//walk

			m_pSkinnedAnimationController->SetTrackAnimationSet(5, BASIC_ANI::Attack(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//attack	//Archer:6, Fighter:18, SwordMan:37, Wizard:51 
			m_pSkinnedAnimationController->SetTrackAnimationSet(14, BASIC_ANI::Death(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()])));	//attack	//Archer:6, Fighter:18, SwordMan:37, Wizard:51 

			for (int i = 6; i < 10; ++i)
			{
				//It's a vector index, so it's subtracted by 1 from the original value
				int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()][i - 6] - PLAYER_SKILL / 4; 
				vector <int> vecSkill = SceneManager::GetInstance()->m_MatchingAniList[skillNum];
				
				if (vecSkill.size() != 0) {
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, vecSkill[0]);
					m_pSkinnedAnimationController->SetDetailSkillAnim_Hero(i, skillNum + 1 , vecSkill[0]);
				}
				else {
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, -1);
				}
				if (vecSkill.size() >= 2)
				{
					m_pSkinnedAnimationController->SetTrackContinuousAni(i, true);
					m_pSkinnedAnimationController->SetTrackAnimationSet(i + 4, vecSkill[1]);
					m_pSkinnedAnimationController->SetDetailSkillAnim_Hero(i + 4, skillNum + 1, vecSkill[1]);
				}
			}
			for (int i = 0; i < AniTrack; ++i)
				m_pSkinnedAnimationController->SetTrackEnable(i, false);
			m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
			m_pSkinnedAnimationController->m_pAnimationTracks[14].m_nType = ANIMATION_TYPE_ONCE;
			for (int i = 1; i < 5; ++i)
				m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(2.0f);
			m_pSkinnedAnimationController->m_pAnimationTracks[1].SetSpeed(1.5f);
			m_pSkinnedAnimationController->m_pAnimationTracks[3].SetSpeed(1.8f);
			//m_pSkinnedAnimationController->m_pAnimationTracks[3].SetSpeed(2.f);
			if (static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()]) == JOB::FIGHTER )
				m_pSkinnedAnimationController->m_pAnimationTracks[5].SetSpeed(2.f);
			else if(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()]) == JOB::SWORDMAN)
				m_pSkinnedAnimationController->m_pAnimationTracks[5].SetSpeed(2.f);

			//m_pSkinnedAnimationController->SetCallbackKeys(1, 2);
		}
		else
		{

			if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
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
					int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()][i - 6] - BOSS_SKILL / 2;
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, BOSS_OGRE_ANI::Skill_AniNum(skillNum));
					m_pSkinnedAnimationController->SetDetailSkillAnim_Boss(i, skillNum);
				}
				for (int i = 0; i < 11; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
				m_pSkinnedAnimationController->m_pAnimationTracks[10].m_nType = ANIMATION_TYPE_ONCE;
				for (int i = 3; i < 5; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);

				m_pSkinnedAnimationController->m_pAnimationTracks[1].SetSpeed(2.0f);
				m_pSkinnedAnimationController->m_pAnimationTracks[2].SetSpeed(2.0f);

			}
			else if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
			{
				m_pSkinnedAnimationController = new CAnimationController(pd3dDevice, pd3dCommandList, 11, pAngrybotModel);
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
					int skillNum = NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()][i - 6] - BOSS_SKILL / 2;
					m_pSkinnedAnimationController->SetTrackAnimationSet(i, BOSS_PROGRAMMER_ANI::Skill_AniNum(skillNum));
					m_pSkinnedAnimationController->SetDetailSkillAnim_Boss(i, skillNum);
				}

				for (int i = 0; i < 11; ++i)
					m_pSkinnedAnimationController->SetTrackEnable(i, false);
				m_pSkinnedAnimationController->m_pAnimationTracks[5].m_nType = ANIMATION_TYPE_ONCE;
				m_pSkinnedAnimationController->m_pAnimationTracks[10].m_nType = ANIMATION_TYPE_ONCE;
				for (int i = 1; i < 5; ++i)
					m_pSkinnedAnimationController->m_pAnimationTracks[i].SetSpeed(1.5f);
			}
		}
		
	}
	
//#ifdef _WITH_SOUND_RESOURCE
//	m_pSkinnedAnimationController->SetCallbackKey(0, 0.1f, _T("Footstep01"));
//	m_pSkinnedAnimationController->SetCallbackKey(1, 0.5f, _T("Footstep02"));
//	m_pSkinnedAnimationController->SetCallbackKey(2, 0.9f, _T("Footstep03"));
//#else
//	m_pSkinnedAnimationController->SetCallbackKey(1, 0, 0.2f, _T("Sound/Footstep01.wav"));
//	m_pSkinnedAnimationController->SetCallbackKey(1, 1, 0.5f, _T("Sound/Footstep02.wav"));
////	m_pSkinnedAnimationController->SetCallbackKey(1, 2, 0.39f, _T("Sound/Footstep03.wav"));
//#endif
//	CAnimationCallbackHandler *pAnimationCallbackHandler = new CSoundCallbackHandler();
//	m_pSkinnedAnimationController->SetAnimationCallbackHandler(1, pAnimationCallbackHandler);

	AllCreateShaderVariables(pd3dDevice, pd3dCommandList);

	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
		if (SceneManager::GetInstance()->GetOrder() == ORDER::BOSS) {
			if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
			{
				SetScale(XMFLOAT3(OGRE_SCALE, OGRE_SCALE, OGRE_SCALE));
				m_pCamera->SetOffset(XMFLOAT3(0.0f, 3.0f, -4.0f));
			}
			else if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
			{
				SetScale(XMFLOAT3(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE));
				m_pCamera->SetOffset(XMFLOAT3(0.0f, 3.0f, -3.5f));
			}
		}
		else {
			//Hero
			SetScale(XMFLOAT3(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE));
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 3.0f, -3.5f));
		}
	}
	else {
		//Lobby
		SetScale(XMFLOAT3(PLAYER_SCALE, PLAYER_SCALE, PLAYER_SCALE));
	}
	
	SetObjectType(OBJ_TYPE::PLAYER);
	if (pAngrybotModel) delete pAngrybotModel;
}

CGamePlayer::~CGamePlayer()
{
}

CCamera *CGamePlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
		case FIRST_PERSON_CAMERA:
			SetFriction(250.0f);
			SetGravity(XMFLOAT3(0.0f, -400.0f, 0.0f));
			SetMaxVelocityXZ(300.0f);
			SetMaxVelocityY(400.0f);
			m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.0f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, 0.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		case SPACESHIP_CAMERA:
			SetFriction(125.0f);
			SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
			SetMaxVelocityXZ(300.0f);
			SetMaxVelocityY(400.0f);
			m_pCamera = OnChangeCamera(SPACESHIP_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.0f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		case THIRD_PERSON_CAMERA:
			SetFriction(WORLD_FRICTION);
			SetGravity(XMFLOAT3(0.0f, WORLD_GRAVITY, 0.0f));
			SetMaxVelocityXZ(PLAYER_MAX_VELXZ);
			SetMaxVelocityY(PLAYER_MAX_VELY);
			m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
			m_pCamera->SetTimeLag(0.25f);
			m_pCamera->SetOffset(XMFLOAT3(0.0f, 2.0f, -3.0f));
			m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
			m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
			m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
			break;
		default:
			break;
	}
	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	m_pCamera->SetPosition(XMFLOAT3(0.0f, 5000.0f, -3000.0f));
	Update(fTimeElapsed);

	return(m_pCamera);
}

void CGamePlayer::OnPlayerUpdateCallback(float fTimeElapsed)
{
	XMFLOAT3 xmf3PlayerPosition = GetPosition();
	if (xmf3PlayerPosition.y < FLOOR_HEIGHT)
	{
		XMFLOAT3 xmf3PlayerVelocity = GetVelocity();
		xmf3PlayerVelocity.y = 0.0f;
		SetVelocity(xmf3PlayerVelocity);
		xmf3PlayerPosition.y = FLOOR_HEIGHT;
		SetPosition(xmf3PlayerPosition);
	}
}

void CGamePlayer::Move(DWORD dwDirection, float fDistance, bool bUpdateVelocity)
{
	if (NetworkManager::GetInstance()->myClient->GetPlayerInfo().Speed >= 1.2f) {
		CTextureShader::GetInstance()->AccelerateTextureSwitch(true);
	}
	else {
		CTextureShader::GetInstance()->AccelerateTextureSwitch(false);
	}

	if (m_status.skillBuff == SKILL_BUFF::STUN) {
		cout << "Stun" << endl;
		m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
		m_pSkinnedAnimationController->SetTrackEnable(0, true);
		return;
	}
	else if (m_jumping) {
		dwDirection = 0x10;
	}

	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::LOBBY &&
		CTextureShader::GetInstance()->IsCustomize()) return;


	if (dwDirection && m_nNowAnimation < 5 && SceneManager::GetInstance()->m_nCurScene != SCENEKIND::READY)
	{
		m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);

		switch (dwDirection)
		{
		case DIR_FORWARD:
		case 0x0D:
		case 0x10:
			m_pSkinnedAnimationController->SetTrackEnable(1, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = 0;
				m_pSkinnedAnimationController->m_iPostTrackNum = 1;
			}
			m_nNowAnimation = 1;
			break;

		case DIR_BACKWARD:
		case 0x06:
		case 0x0A:
		case 0x0E:
			m_pSkinnedAnimationController->SetTrackEnable(2, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = 0;
				m_pSkinnedAnimationController->m_iPostTrackNum = 2;
			}
			m_nNowAnimation = 2;
			break;

		case DIR_LEFT:
		case 0x05:
			m_pSkinnedAnimationController->SetTrackEnable(3, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = 0;
				m_pSkinnedAnimationController->m_iPostTrackNum = 3;
			}
			m_nNowAnimation = 3;
			break;

		case DIR_RIGHT:
		case 0x09:
			m_pSkinnedAnimationController->SetTrackEnable(4, true);
			m_pSkinnedAnimationController->CheckingAniChage();
			if (m_pSkinnedAnimationController->m_bAniChange)
			{
				m_pSkinnedAnimationController->m_bBlend = true;
				m_pSkinnedAnimationController->m_iPreTrackNum = 0;
				m_pSkinnedAnimationController->m_iPostTrackNum = 4;
			}
			m_nNowAnimation = 4;
			break;
			
		default:
			m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, true);
			break;
		}
	}
	else
	{
		fDistance = 0.f;
	}

	if (!m_usingSkill) {
		CPlayer::Move(dwDirection, fDistance, bUpdateVelocity);
	}
}

void CGamePlayer::Update(float fTimeElapsed)
{
	if (m_nNowAnimation < 5 || m_moveAnimNum.contains(m_pSkinnedAnimationController->GetTrackAnimationSet(m_nNowAnimation))) {//Move, MoveAnim

		XMFLOAT3 targetPosition = XMFLOAT3(NetworkManager::GetInstance()->myInfo->x, NetworkManager::GetInstance()->myInfo->y, NetworkManager::GetInstance()->myInfo->z);
		XMFLOAT3 currentPosition = GetPosition();
		float distance = sqrt(pow(targetPosition.x - currentPosition.x, 2.f) + pow(targetPosition.y - currentPosition.y, 2.f) + pow(targetPosition.z - currentPosition.z, 2.f));

		long long delay = abs(std::chrono::high_resolution_clock::now().time_since_epoch().count() - NetworkManager::GetInstance()->myInfo->lastPacketTime) - std::chrono::nanoseconds(15000000).count();
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
			XMFLOAT3 previousPosition = XMFLOAT3(NetworkManager::GetInstance()->myInfo->prevX, NetworkManager::GetInstance()->myInfo->prevY, NetworkManager::GetInstance()->myInfo->prevZ);
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

			SetPosition(newPosition);
		}
	}
	if (!CTextureShader::GetInstance()->IsCustomize()) {
		XMFLOAT3 cameraRight = XMFLOAT3(1.0f, 0.0f, 0.0f);
		XMFLOAT3 playerPos = GetPosition();
		XMFLOAT3 newPos = XMFLOAT3(playerPos.x + 0.7f,
			playerPos.y + 0.6,
			playerPos.z);

		m_customizePos = newPos;
	}
	DWORD nCurrentCameraMode = m_pCamera->GetMode();
	XMFLOAT3 playerPos = GetPosition();
	playerPos.y += 2.0f;
	if (nCurrentCameraMode == THIRD_PERSON_CAMERA || nCurrentCameraMode == READY_SCENE_CAMERA) {
		if (!CTextureShader::GetInstance()->IsCustomize()) {
			m_pCamera->Update(m_xmf3Position, fTimeElapsed);
		}
		else {
			m_pCamera->Update(m_customizePos, fTimeElapsed); 
		}
		
	}

	if (nCurrentCameraMode == THIRD_PERSON_CAMERA || nCurrentCameraMode == READY_SCENE_CAMERA)
	{
		if (!CTextureShader::GetInstance()->IsCustomize()) {
			m_pCamera->SetLookAt(playerPos);
		}
		else {
			m_pCamera->SetLookAt(m_customizePos);
		}
	}
	m_pCamera->RegenerateViewMatrix();
	Frustum::GetInstance()->m_xmfCamera4x4View = m_pCamera->GetViewMatrix();
	Frustum::GetInstance()->m_xmf4x4CameraProjection = m_pCamera->GetProjectionMatrix();
	Frustum::GetInstance()->Update();

	if (NetworkManager::GetInstance()->skillUsed) {
		UseSkill(NetworkManager::GetInstance()->playerSkill);
		NetworkManager::GetInstance()->skillUsed = false;
	}

	if (m_Ani != ANI_ON_SERVER::NONE)
	{
		if (m_Ani == ANI_ON_SERVER::DEAD)
			SetDeathAnim();
		else if (m_Ani == ANI_ON_SERVER::IDLE)
			SetIdleAnim();
	}

	if (m_pSkinnedAnimationController)
	{
		//m_pSkinnedAnimationController->CheckingAniChage();
		if (m_nNowAnimation >= 5)
		{
			if (m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_nType != ANIMATION_TYPE_LOOP)
			{
				if (m_pSkinnedAnimationController->IsAnimationFinished(m_nNowAnimation))
				{
					int DeathNum = (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) ? 14 : 10;
					if (m_nNowAnimation != DeathNum)
					{
						if (m_pSkinnedAnimationController->GetTrackContinuousAni(m_nNowAnimation))
						{
							m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
							m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
							m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation + 4, true);
							m_pSkinnedAnimationController->CheckingAniChage();
							if (m_pSkinnedAnimationController->m_bAniChange)
							{
								m_pSkinnedAnimationController->m_bBlend = true;
								m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
								m_pSkinnedAnimationController->m_iPostTrackNum = m_nNowAnimation + 4;
							}
							m_nNowAnimation = m_nNowAnimation + 4;
							//NetworkManager::GetInstance()->SendSkillFinishPacket();
							m_usingSkill = true;
						}
						else
						{
							m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
							m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
							m_pSkinnedAnimationController->SetTrackEnable(0, true);
							m_pSkinnedAnimationController->CheckingAniChage();
							if (m_pSkinnedAnimationController->m_bAniChange)
							{
								m_pSkinnedAnimationController->m_bBlend = true;
								m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
								m_pSkinnedAnimationController->m_iPostTrackNum = 0;
							}
							m_nNowAnimation = 0;
							NetworkManager::GetInstance()->SendSkillFinishPacket();
							m_usingSkill = false;
						}
					}
				}
			}
			else
			{
				if (m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_bLoop == false)
				{
					m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
					m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
					m_pSkinnedAnimationController->SetTrackEnable(0, true);
					m_pSkinnedAnimationController->CheckingAniChage();
					if (m_pSkinnedAnimationController->m_bAniChange)
					{
						m_pSkinnedAnimationController->m_bBlend = true;
						m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
						m_pSkinnedAnimationController->m_iPostTrackNum = 0;
					}
					m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_bLoop = true;
					m_nNowAnimation = 0;
					NetworkManager::GetInstance()->SendSkillFinishPacket();
					m_usingSkill = false;				
				}
				else
				{
					if (m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_bOnOff == true)
					{
						if (m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_bOnOffToggle == false)
						{
							m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
							m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
							m_pSkinnedAnimationController->SetTrackEnable(0, true);
							m_pSkinnedAnimationController->CheckingAniChage();
							if (m_pSkinnedAnimationController->m_bAniChange)
							{
								m_pSkinnedAnimationController->m_bBlend = true;
								m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
								m_pSkinnedAnimationController->m_iPostTrackNum = 0;
							}
							m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_fAnimLoopTime = 0.f;
							m_pSkinnedAnimationController->m_pAnimationTracks[m_nNowAnimation].m_iAnimLoopCount = 0;

							m_nNowAnimation = 0;
							NetworkManager::GetInstance()->SendSkillFinishPacket();
							m_usingSkill = false;
						}
					}
				}
			}
			
		}
		else if (m_dir == 0)
		{
			if (SceneManager::GetInstance()->m_nCurScene != SCENEKIND::READY)
			{
				if (m_nNowAnimation) {
					m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
					m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
				}
				m_pSkinnedAnimationController->SetTrackEnable(0, true);
				m_pSkinnedAnimationController->CheckingAniChage();
				if (m_pSkinnedAnimationController->m_bAniChange)
				{
					m_pSkinnedAnimationController->m_bBlend = true;
					m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
					m_pSkinnedAnimationController->m_iPostTrackNum = 0;
				}
				m_nNowAnimation = 0;
			}
		}
	}
}

void CGamePlayer::SetReadyAnim(int ChangeJob)
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::READY)
	{
		if (m_pSkinnedAnimationController)
		{
			m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
			m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
			m_pSkinnedAnimationController->SetTrackEnable(ChangeJob, true);
			m_nNowAnimation = ChangeJob;
		}
	}
}

void CGamePlayer::SetDeathAnim()
{
	if (m_pSkinnedAnimationController)
	{
		int DeathNum = (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS) ? 14 : 10;
		m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
		m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
		m_pSkinnedAnimationController->SetTrackEnable(DeathNum, true);
		m_pSkinnedAnimationController->CheckingAniChage();
		if (m_pSkinnedAnimationController->m_bAniChange)
		{
			m_pSkinnedAnimationController->m_bBlend = true;
			m_pSkinnedAnimationController->m_iPreTrackNum = m_nNowAnimation;
			m_pSkinnedAnimationController->m_iPostTrackNum = DeathNum;
		}
		m_nNowAnimation = DeathNum;
	}
	m_Ani = ANI_ON_SERVER::NONE;
}

void CGamePlayer::SetIdleAnim()
{
	if (m_pSkinnedAnimationController)
	{
		m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
		m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
		m_pSkinnedAnimationController->SetTrackEnable(0, true);
		m_nNowAnimation = 0;
	}
	m_Ani = ANI_ON_SERVER::NONE;
}

bool CGamePlayer::CheckCoolTime(SKILLKIND skillNum)
{
	if (chrono::duration_cast<chrono::seconds>(chrono::system_clock::now() - NetworkManager::GetInstance()->myInfo->lastSkillTime[static_cast<int>(skillNum) - 1]).count() >= m_skillCoolTime[static_cast<int>(skillNum) - 1]) {
		return true;
	}
	return false;
}

void CGamePlayer::LoadCoolTime()
{
	for (int i = 0; i < 5; ++i) {
		m_skillCoolTime[i] = NetworkManager::GetInstance()->skillCoolTime[i];
	}
}

void CGamePlayer::UpdateRemaingTime()
{
	for (int i = 0; i < UILayer::GetInstance()->m_skillCoolRemainingTime.size(); ++i)
	{
		if (UILayer::GetInstance()->m_skillCoolRemainingTime[i] > 0.f)
		{			
			auto passedTime = chrono::duration_cast<chrono::milliseconds>(chrono::system_clock::now() - NetworkManager::GetInstance()->myInfo->lastSkillTime[i]).count();
			UILayer::GetInstance()->m_skillCoolRemainingTime[i] = m_skillCoolTime[i] - static_cast<int>(std::floorf(passedTime / 1000.0f));
		}
	}
}

void CGamePlayer::UseSkillCheck(SKILLKIND nSkillNum)
{
	if (SceneManager::GetInstance()->m_nCurScene == SCENEKIND::INGAME) {
		if (CTextureShader::GetInstance()->IsShopping())
			return;

#ifdef Test
		if (m_status.skillBuff == SKILL_BUFF::SILENCE && m_status.skillBuff == SKILL_BUFF::STUN)
			return;
#endif
		if (m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOff)
		{
			if (m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle)//true
			{
				m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle = !m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle;
				UILayer::GetInstance()->m_skillCoolRemainingTime[static_cast<int>(nSkillNum) - 1] = m_skillCoolTime[static_cast<int>(nSkillNum) - 1];
				NetworkManager::GetInstance()->SendSkillPacket(nSkillNum, true);
				cout << "toggle skill off" << endl;
				for (auto p : SceneManager::GetInstance()->m_ParticleInfo[static_cast<PARTICLE_SITUATION>(NetworkManager::GetInstance()->GetId())][static_cast<int>(nSkillNum) - 1])
				{
					if (p->show)
					{
						p->show = false;
					}
				}
			}
			else //false
			{
				if (m_status.skillBuff != SKILL_BUFF::SILENCE && CheckCoolTime(nSkillNum)) {
					NetworkManager::GetInstance()->SendSkillPacket(nSkillNum);
					m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle = !m_pSkinnedAnimationController->m_pAnimationTracks[static_cast<int>(nSkillNum) + 4].m_bOnOffToggle;
					cout << "toggle skill on" << endl;
				}
			}			
		}
		else
		{
			if (m_status.skillBuff != SKILL_BUFF::SILENCE && CheckCoolTime(nSkillNum)) {
				NetworkManager::GetInstance()->SendSkillPacket(nSkillNum);
			}
		}
		

	}
}

void CGamePlayer::UseSkill(SKILLKIND nSkillNum)
{
	if (m_pSkinnedAnimationController)
	{
		for (auto p : SceneManager::GetInstance()->m_ParticleInfo[static_cast<PARTICLE_SITUATION>(NetworkManager::GetInstance()->GetId())][static_cast<int>(nSkillNum) - 1])
		{
			if (!p->show)
			{
				p->show = true;
				p->Dir = GetLookVector();
				p->pos = GetPosition();
			}
			else
			{
				p->show = false;
			}
		}

		if (m_pSkinnedAnimationController->GetTrackAnimationSet(static_cast<int>(nSkillNum) + 4) != -1)
		{
			m_dir = 0;
			m_usingSkill = true;
			if (m_nNowAnimation < 5) {
				m_pSkinnedAnimationController->SetTrackEnable(m_nNowAnimation, false);
				m_pSkinnedAnimationController->SetTrackPosition(m_nNowAnimation, 0.0f);
				m_pSkinnedAnimationController->SetTrackEnable(static_cast<int>(nSkillNum) + 4, true);
				m_pSkinnedAnimationController->CheckingAniChage();
				if (m_pSkinnedAnimationController->m_bAniChange)
				{
					m_pSkinnedAnimationController->m_bBlend = true;
					m_pSkinnedAnimationController->m_iPreTrackNum = 0;
					m_pSkinnedAnimationController->m_iPostTrackNum = static_cast<int>(nSkillNum) + 4;
				}
				m_nNowAnimation = static_cast<int>(nSkillNum) + 4;
				
				//Skill Anim Sound
				if (nSkillNum == SKILLKIND::LEFTCLICK) {
					if (SceneManager::GetInstance()->GetOrder() != ORDER::BOSS)
						SoundManager::GetInstance()->Play_Sound(NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()], CHANNELID::PLAYER);
					else if (SceneManager::GetInstance()->GetOrder() == ORDER::BOSS && NetworkManager::GetInstance()->readySceneInfo->playerJobs[NetworkManager::GetInstance()->GetId()] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE)) {
						SoundManager::GetInstance()->Play_Sound(static_cast<int>(BOSSJOB::OGRE), CHANNELID::PLAYER);
					}
				}
				else {
					SoundManager::GetInstance()->Play_Sound(NetworkManager::GetInstance()->readySceneInfo->selectSkills[NetworkManager::GetInstance()->GetId()][static_cast<int>(nSkillNum) - 2], CHANNELID::PLAYER);
				}
			}
		}
		else
			NetworkManager::GetInstance()->SendSkillFinishPacket();

	}
		
}

bool CGamePlayer::IsBagFull()
{
	for (int i = 0; i < 5; ++i)
	{
		if (m_itemList[i] == ITEMKIND::NONE) return false;
	}
	return true;
}

void CGamePlayer::SetItem(ITEMKIND item)
{
	for (int i = 0; i < 5; ++i)
	{
		if (m_itemList[i] == ITEMKIND::NONE) {
			m_itemList[i] = item;
			CTextureShader::GetInstance()->SetItemState(i, true, item);
			break;
		}
	}
}

void CGamePlayer::UseItem(int index)
{
	if (m_itemList[index] != ITEMKIND::NONE) {
		NetworkManager::GetInstance()->SendUseItemPacket(index);
		m_itemList[index] = ITEMKIND::NONE;
		CTextureShader::GetInstance()->SetItemState(index, false, ITEMKIND::NONE);
		SoundManager::GetInstance()->Play_Sound(L"UseItem.wav", CHANNELID::EFFECT, 0.7f);
	}
}

void CGamePlayer::StatLevelUp(int index)
{
	switch (index)
	{
	case 0: m_statLevels.hp += 1; m_statLevels.hp_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.hp_price, m_statLevels.hp)); break;
	case 1: m_statLevels.mp += 1; m_statLevels.mp_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.mp_price, m_statLevels.mp)); break;
	case 2: m_statLevels.attack += 1; m_statLevels.attack_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.attack_price, m_statLevels.attack)); break;
	case 3: m_statLevels.magic_attack += 1; m_statLevels.magic_attack_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.magic_attack_price, m_statLevels.magic_attack)); break;
	case 4: m_statLevels.defense += 1; m_statLevels.defense_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.defense_price, m_statLevels.defense)); break;
	case 5: m_statLevels.magic_defense += 1; m_statLevels.magic_defense_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.magic_defense_price, m_statLevels.magic_defense)); break;
	case 6: m_statLevels.speed += 1; m_statLevels.speed_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.speed_price, m_statLevels.speed)); break;
	case 7: m_statLevels.tenacity += 1; m_statLevels.tenacity_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.tenacity_price, m_statLevels.tenacity)); break;
	case 8: m_statLevels.critical += 1; m_statLevels.critical_price = static_cast<int>(GET_SHOP_PRICE(m_statLevels.critical_price, m_statLevels.critical)); break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CTitlePlayer::CTitlePlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, void* pContext)
{
	m_pCamera = ChangeCamera(/*SPACESHIP_CAMERA*/FIRST_PERSON_CAMERA, 0.0f);

	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	AddRef();
}

CTitlePlayer::~CTitlePlayer()
{
}

CCamera* CTitlePlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
	case FIRST_PERSON_CAMERA:
		SetFriction(2.0f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(2.5f);
		SetMaxVelocityY(40.0f);
		m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, -10.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	case SPACESHIP_CAMERA:
		SetFriction(100.5f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(40.0f);
		SetMaxVelocityY(40.0f);
		m_pCamera = OnChangeCamera(SPACESHIP_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	case THIRD_PERSON_CAMERA:
		SetFriction(20.5f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(25.5f);
		SetMaxVelocityY(20.0f);
		m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.25f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 15.0f, -30.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	default:
		break;
	}
	m_pCamera->SetPosition(Vector3::Add(m_xmf3Position, m_pCamera->GetOffset()));
	Update(fTimeElapsed);

	return(m_pCamera);
}

CReadyPlayer::CReadyPlayer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, CLoadedModelInfo* pModel, void* pContext) : CGamePlayer(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, pModel, pContext)
{
	m_pCamera = ChangeCamera(READY_SCENE_CAMERA, 0.0f);
	//SetPosition(XMFLOAT3(0, 2.0f, 0));
}

CReadyPlayer::~CReadyPlayer()
{
}

CCamera* CReadyPlayer::ChangeCamera(DWORD nNewCameraMode, float fTimeElapsed)
{
	DWORD nCurrentCameraMode = (m_pCamera) ? m_pCamera->GetMode() : 0x00;
	if (nCurrentCameraMode == nNewCameraMode) return(m_pCamera);
	switch (nNewCameraMode)
	{
	case FIRST_PERSON_CAMERA:
		SetFriction(250.0f);
		SetGravity(XMFLOAT3(0.0f, -400.0f, 0.0f));
		SetMaxVelocityXZ(300.0f);
		SetMaxVelocityY(400.0f);
		m_pCamera = OnChangeCamera(FIRST_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 20.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	case SPACESHIP_CAMERA:
		SetFriction(125.0f);
		SetGravity(XMFLOAT3(0.0f, 0.0f, 0.0f));
		SetMaxVelocityXZ(300.0f);
		SetMaxVelocityY(400.0f);
		m_pCamera = OnChangeCamera(SPACESHIP_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.0f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 0.0f, 0.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	case THIRD_PERSON_CAMERA:
		SetFriction(WORLD_FRICTION);
		SetGravity(XMFLOAT3(0.0f, WORLD_GRAVITY, 0.0f));
		SetMaxVelocityXZ(PLAYER_MAX_VELXZ);
		SetMaxVelocityY(PLAYER_MAX_VELY);
		m_pCamera = OnChangeCamera(THIRD_PERSON_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.25f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, 2.0f, -3.0f));
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		break;
	case READY_SCENE_CAMERA:
		SetFriction(WORLD_FRICTION);
		SetGravity(XMFLOAT3(0.0f, 0.0, 0.0f));
		SetMaxVelocityXZ(PLAYER_MAX_VELXZ);
		SetMaxVelocityY(PLAYER_MAX_VELY);
		m_pCamera = OnChangeCamera(READY_SCENE_CAMERA, nCurrentCameraMode);
		m_pCamera->SetTimeLag(0.25f);
		m_pCamera->SetOffset(XMFLOAT3(0.0f, -0.5f, 5.0f));
		if(SceneManager::GetInstance()->GetOrder() == ORDER::BOSS)
			m_pCamera->Rotate(-3.f, 40.5f, 0.f);
		else
			m_pCamera->Rotate(-3.f, 33.5f, 0.f);
		m_pCamera->GenerateProjectionMatrix(1.01f, 5000.0f, ASPECT_RATIO, 60.0f);
		m_pCamera->SetViewport(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT, 0.0f, 1.0f);
		m_pCamera->SetScissorRect(0, 0, FRAME_BUFFER_WIDTH, FRAME_BUFFER_HEIGHT);
		
		break;
	default:
		break;
	}
	if (SceneManager::GetInstance()->GetOrder() == ORDER::BOSS)
		m_pCamera->SetPosition(Vector3::Add(XMFLOAT3(-25, 5,30), m_pCamera->GetOffset()));
	else
		m_pCamera->SetPosition(Vector3::Add(XMFLOAT3(-37, 5, 27), m_pCamera->GetOffset()));
	Update(fTimeElapsed);

	return(m_pCamera);
}
