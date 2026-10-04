//-----------------------------------------------------------------------------
// File: CScene.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Scene.h"
#include "NetworkManager.h"
#include "SceneManager.h"
#include "UILayer.h"
//#include "Shader.h"
#include "CSkillModel.h"
#include "SoundManager.h"
#include "Util.h"

namespace PARTICLE_SKILLSETTING
{
	int NumParticle(int SkillNum);
	int NumParticle(SKILL_TYPE Type);
	PARTICLE_TYPE Type(int SkillNum, int NumParticle = 1);
	PARTICLE_TYPE Type(SKILL_TYPE Type, int NumParticle = 1);
	int TextureAddress(int SkillNum, int NumParticle = 1);
	int TextureAddress(SKILL_TYPE Type, int NumParticle = 1);
}


ID3D12DescriptorHeap *CScene::m_pd3dCbvSrvDescriptorHeap = NULL;

D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvCPUDescriptorStartHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvGPUDescriptorStartHandle;
D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvCPUDescriptorStartHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvGPUDescriptorStartHandle;

D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvCPUDescriptorNextHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dCbvGPUDescriptorNextHandle;
D3D12_CPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvCPUDescriptorNextHandle;
D3D12_GPU_DESCRIPTOR_HANDLE	CScene::m_d3dSrvGPUDescriptorNextHandle;

const wchar_t* CIngameScene::ParticleTextureAddress[ADDRESS_COUNT] = { L"Image/Effect/RoundSoftParticle.dds" , L"Image/Effect/arrow.dds", L"Image/Effect/Noise1.dds" , L"Image/Effect/Fog.dds", L"Image/Effect/Attack.dds", L"Image/Effect/Counter.dds", L"Image/Effect/shield.dds", L"Image/Effect/Blade.dds", L"Image/Effect/sword.dds", L"Image/Effect/reflect.dds", L"Image/Effect/BlackSphere.dds", L"Image/Effect/Dimension.dds", L"Image/Effect/HellBlade.dds", L"Image/Effect/coin.dds", L"Image/Effect/Electronic.dds" };

CScene::CScene()
{
}

CScene::~CScene()
{
}

void CScene::BuildDefaultLightsAndMaterials()
{

}

void CScene::BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, ID3D12RootSignature* m_pd3dGraphicsRootSignature)
{
}

void CScene::BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel)
{
	m_ppOtherClient = new CGameObject * [LOBBY_MAX_CLIENT];
	for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
		m_ppOtherClient[i] = new COtherClientPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pModel, 0);
		NetworkManager::GetInstance()->OtherClients[i] = m_ppOtherClient[i];
	}
}

void CScene::ReleaseObjects()
{
	if (m_pd3dCbvSrvDescriptorHeap) {
		int ref = m_pd3dCbvSrvDescriptorHeap->Release();
		if (ref) {
			cout << "Scene, m_pd3dCbvSrvDescriptorHeap Ref: " << ref << endl;
		}
		else
			m_pd3dCbvSrvDescriptorHeap = nullptr;
	}

	if (m_ppShaders)
	{
		for (int i = 0; i < m_nShaders; i++)
		{
			m_ppShaders[i]->ReleaseShaderVariables();
			m_ppShaders[i]->ReleaseObjects();
			m_ppShaders[i]->Release();
			m_ppShaders[i] = nullptr;
		}
		delete[] m_ppShaders;
		m_ppShaders = nullptr;
	}

	if (m_pSkyBox) {
		delete m_pSkyBox;
		m_pSkyBox = nullptr;
	}

	if (m_ppHierarchicalGameObjects)
	{
		for (int i = 0; i < m_nHierarchicalGameObjects; i++) 
			if (m_ppHierarchicalGameObjects[i]) {
				m_ppHierarchicalGameObjects[i]->Release();
				cout << "=========================================" << endl;
			}
		delete[] m_ppHierarchicalGameObjects;
	}

	ReleaseShaderVariables();

	if (m_pLights) {
		delete[] m_pLights;
		m_pLights = nullptr;
	}
}

void CScene::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(LIGHTS) + 255) & ~255); //256의 배수
	m_pd3dcbLights = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	m_pd3dcbLights->Map(0, NULL, (void **)&m_pcbMappedLights);

	UINT ncbElementBytesScene = ((sizeof(VS_CB_SCENE_STATE) + 255) & ~255); //256의 배수
	m_pd3dcbScene = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytesScene, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	m_pd3dcbScene->Map(0, NULL, (void**)&m_pcbMappedScene);
}

void CScene::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); //Lights

	::memcpy(m_pcbMappedLights->m_pLights, m_pLights, sizeof(LIGHT) * m_nLights);
	::memcpy(&m_pcbMappedLights->m_xmf4GlobalAmbient, &m_xmf4GlobalAmbient, sizeof(XMFLOAT4));
	::memcpy(&m_pcbMappedLights->m_nLights, &m_nLights, sizeof(int));

	D3D12_GPU_VIRTUAL_ADDRESS d3dcbSceneStateGpuVirtualAddress = m_pd3dcbScene->GetGPUVirtualAddress();
	pd3dCommandList->SetGraphicsRootConstantBufferView(15, d3dcbSceneStateGpuVirtualAddress); //SceneState
	::memcpy(&m_pcbMappedScene->m_nDrawOption, &m_nDrawOption, sizeof(UINT));
	::memcpy(&m_pcbMappedScene->m_fExposure, &m_fExposure, sizeof(float));
	::memcpy(&m_pcbMappedScene->m_fSaturation, &m_fSaturation, sizeof(float));
	::memcpy(&m_pcbMappedScene->m_fContrast, &m_fContrast, sizeof(float));
	::memcpy(&m_pcbMappedScene->m_fVibrance, &m_fVibrance, sizeof(float));
	::memcpy(&m_pcbMappedScene->m_nCurScene, &m_nCurScene, sizeof(UINT));
	::memcpy(&m_pcbMappedScene->m_outline, &m_outline, sizeof(bool));
}

void CScene::ReleaseShaderVariables()
{
	if (m_pd3dcbLights)
	{
		m_pd3dcbLights->Unmap(0, NULL);
		m_pd3dcbLights->Release();
	}

	if (m_pd3dcbScene)
	{
		m_pd3dcbScene->Unmap(0, NULL);
		m_pd3dcbScene->Release();
	}
}

void CScene::ReleaseUploadBuffers()
{
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nHierarchicalGameObjects; i++) m_ppHierarchicalGameObjects[i]->ReleaseUploadBuffers();
}

void CScene::CreateCbvSrvDescriptorHeaps(ID3D12Device *pd3dDevice, int nConstantBufferViews, int nShaderResourceViews)
{
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = nConstantBufferViews + nShaderResourceViews; //CBVs + SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	pd3dDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void **)&m_pd3dCbvSrvDescriptorHeap);

	m_d3dCbvCPUDescriptorNextHandle = m_d3dCbvCPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dCbvGPUDescriptorNextHandle = m_d3dCbvGPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	m_d3dSrvCPUDescriptorNextHandle.ptr = m_d3dSrvCPUDescriptorStartHandle.ptr = m_d3dCbvCPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
	m_d3dSrvGPUDescriptorNextHandle.ptr = m_d3dSrvGPUDescriptorStartHandle.ptr = m_d3dCbvGPUDescriptorStartHandle.ptr + (::gnCbvSrvDescriptorIncrementSize * nConstantBufferViews);
}

D3D12_GPU_DESCRIPTOR_HANDLE CScene::CreateConstantBufferViews(ID3D12Device *pd3dDevice, int nConstantBufferViews, ID3D12Resource *pd3dConstantBuffers, UINT nStride)
{
	D3D12_GPU_DESCRIPTOR_HANDLE d3dCbvGPUDescriptorHandle = m_d3dCbvGPUDescriptorNextHandle;
	D3D12_GPU_VIRTUAL_ADDRESS d3dGpuVirtualAddress = pd3dConstantBuffers->GetGPUVirtualAddress();
	D3D12_CONSTANT_BUFFER_VIEW_DESC d3dCBVDesc;
	d3dCBVDesc.SizeInBytes = nStride;
	for (int j = 0; j < nConstantBufferViews; j++)
	{
		d3dCBVDesc.BufferLocation = d3dGpuVirtualAddress + (nStride * j);
		m_d3dCbvCPUDescriptorNextHandle.ptr = m_d3dCbvCPUDescriptorNextHandle.ptr + ::gnCbvSrvDescriptorIncrementSize;
		pd3dDevice->CreateConstantBufferView(&d3dCBVDesc, m_d3dCbvCPUDescriptorNextHandle);
		m_d3dCbvGPUDescriptorNextHandle.ptr = m_d3dCbvGPUDescriptorNextHandle.ptr + ::gnCbvSrvDescriptorIncrementSize;
	}
	return(d3dCbvGPUDescriptorHandle);
}

void CScene::CreateShaderResourceViews(ID3D12Device* pd3dDevice, CTexture* pTexture, UINT nDescriptorHeapIndex, UINT nRootParameterStartIndex)
{
	m_d3dSrvCPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);
	m_d3dSrvGPUDescriptorNextHandle.ptr += (::gnCbvSrvDescriptorIncrementSize * nDescriptorHeapIndex);

	if (pTexture)
	{
		int nTextures = pTexture->GetTextures();
		for (int i = 0; i < nTextures; i++)
		{
			ID3D12Resource* pShaderResource = pTexture->GetResource(i);
			D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = pTexture->GetShaderResourceViewDesc(i);
			pd3dDevice->CreateShaderResourceView(pShaderResource, &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
			m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
			pTexture->SetGpuDescriptorHandle(i, m_d3dSrvGPUDescriptorNextHandle);
			m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
		}
	}
	int nRootParameters = pTexture->GetRootParameters();
	for (int j = 0; j < nRootParameters; j++) pTexture->SetRootParameterIndex(j, nRootParameterStartIndex + j);
}

bool CScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	return(false);
}

bool CScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		break;
	default:
		break;
	}
	return(false);
}

bool CScene::ProcessInput(UCHAR *pKeysBuffer)
{
	return(false);
}

void CScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Animate(fTimeElapsed);
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);

}

void CScene::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera, bool bIsAnimated)
{
	m_outline = SceneManager::GetInstance()->m_outline;
	m_nDrawOption = SceneManager::GetInstance()->m_nDrawOption;
	m_fExposure = SceneManager::GetInstance()->m_fContrast;
	m_fSaturation = SceneManager::GetInstance()->m_fSaturation;
	m_fContrast = SceneManager::GetInstance()->m_fContrast;
	m_fVibrance = SceneManager::GetInstance()->m_fVibrance;

	if (m_pSkyBox) m_pSkyBox->Render(pd3dCommandList, pCamera);	

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Render(pd3dCommandList, pCamera);
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->Render(pd3dCommandList, pCamera);

	
	for (int i = 0; i < m_nHierarchicalGameObjects; i++)
	{
		if (m_ppHierarchicalGameObjects[i])
		{
			if(bIsAnimated)m_ppHierarchicalGameObjects[i]->Animate(m_fElapsedTime);
			if (!m_ppHierarchicalGameObjects[i]->m_pSkinnedAnimationController) m_ppHierarchicalGameObjects[i]->UpdateTransform(NULL);
			m_ppHierarchicalGameObjects[i]->Render(pd3dCommandList, pCamera);
		}
	}
	if (m_pPlayer) {
		if(bIsAnimated)m_pPlayer->Animate(m_fElapsedTime);
		m_pPlayer->UpdateTransform(NULL);
		m_pPlayer->Update(m_fElapsedTime);
		m_pPlayer->SetDissolveState(m_pPlayer->m_nObjectDissolveState);
		if (NetworkManager::GetInstance()->myInfo->dissolve)
		{
			m_pPlayer->SetAddDissolveState(m_fElapsedTime * 2);
			m_pPlayer->m_fDissolveTime += m_fElapsedTime;
			if (m_pPlayer->m_fDissolveTime > 3.5f)
			{
				NetworkManager::GetInstance()->myInfo->show = false;
				m_pPlayer->m_fDissolveTime = 0.f;
				m_pPlayer->m_nObjectDissolveState = 0;
			}
		}
		m_pPlayer->Render(pd3dCommandList, pCamera, m_pPlayer->iMyShareNum);
	}
}

void CScene::RenderParticle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	//Particle update
	for (auto pMap : m_ParticleObjects)
	{
		for (int skillNum = 0; skillNum < pMap.second.size(); ++skillNum)
		{
			for (int j = 0; j < pMap.second[skillNum].size(); ++j)
			{
				if (pMap.second[skillNum][j])
				{
					pMap.second[skillNum][j]->SetShow(SceneManager::GetInstance()->m_ParticleInfo[pMap.first][skillNum][j]->show);
					if (pMap.second[skillNum][j]->GetShow())
					{
						if (pMap.second[skillNum][j]->GetUpdateRotate())
						{
							pMap.second[skillNum][j]->Rotate(0, 2, 0);
						}
						if (pMap.second[skillNum][j]->GetUpdatePosition())
						{
							if (NetworkManager::GetInstance()->GetId() == static_cast<int>(pMap.first))
							{
								pMap.second[skillNum][j]->SetPosition(m_pPlayer->GetPosition());
							}
							else
							{
								pMap.second[skillNum][j]->SetPosition(m_ppOtherClient[static_cast<int>(pMap.first)]->GetPosition());
							}
						}
						else
						{
							pMap.second[skillNum][j]->SetPosition(SceneManager::GetInstance()->m_ParticleInfo[pMap.first][skillNum][j]->pos);
						}
						pMap.second[skillNum][j]->SetForwardVector(SceneManager::GetInstance()->m_ParticleInfo[pMap.first][skillNum][j]->Dir);
						SceneManager::GetInstance()->m_ParticleInfo[pMap.first][skillNum][j]->show = pMap.second[skillNum][j]->AnimateLifeTime(m_fElapsedTime * 2);
					}			
				}
			}
		}
	}


	for (auto p : m_ParticleObjects)
	{
		for (int i = 0; i < p.second.size(); ++i)
		{
			for (auto q : p.second[i])
			{
				if (q)
				{
					if (q->GetShow())
					{
						q->Render(pd3dCommandList, pCamera);
					}
						
				}
			}

		}
	}

	for (auto Map : m_SkillTypeParticle)
	{
		for (auto Particle : Map.second)
		{
			if (Particle)
			{
				if(Particle->GetShow())
					Particle->Render(pd3dCommandList, pCamera);
			}
		}
	}
}

void CScene::OnPostRenderParticle()
{

	for (auto p : m_ParticleObjects)
	{
		for (int i = 0; i < p.second.size(); ++i)
		{
			for (auto q : p.second[i])
			{
				if (q)
				{
					if (q->GetShow())
						q->OnPostRender();
				}
			}

		}
	}

	for (auto Map : m_SkillTypeParticle)
	{
		for (auto Particle : Map.second)
		{
			if (Particle)
			{
				if (Particle->GetShow())
					Particle->OnPostRender();
			}
		}
	}
}

void CScene::OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimate)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap) pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);

	pCamera->SetViewportsAndScissorRects(pd3dCommandList);
	if(bIsAnimate)pCamera->UpdateShaderVariables(pd3dCommandList);

	UpdateShaderVariables(pd3dCommandList);

	//D3D12_GPU_VIRTUAL_ADDRESS d3dcbLightsGpuVirtualAddress = m_pd3dcbLights->GetGPUVirtualAddress();
	//pd3dCommandList->SetGraphicsRootConstantBufferView(2, d3dcbLightsGpuVirtualAddress); //Lights
}

void CScene::OnMTPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_pd3dGraphicsRootSignature) pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature);
	if (m_pd3dCbvSrvDescriptorHeap) pd3dCommandList->SetDescriptorHeaps(1, &m_pd3dCbvSrvDescriptorHeap);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CTitleScene::CTitleScene()
{
}

CTitleScene::~CTitleScene()
{
}

bool CTitleScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);

		UILayer::GetInstance()->ProcessMouseClick(SCENEKIND::TITLE, clickPos);
		CTextureShader::GetInstance()->OnMouseClick();
		break;
	}
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);
		CTextureShader::GetInstance()->OnMouseRelease();
		break;
	}
	case WM_MOUSEMOVE:
	{
		POINT clickPos;
		clickPos.x = GET_X_LPARAM(lParam);
		clickPos.y = GET_Y_LPARAM(lParam);
		CTextureShader::GetInstance()->OnMouseMoved(static_cast<float>(clickPos.x), static_cast<float>(clickPos.y));
		break;
	}
	default:
		break;
	}
	return false;
}

bool CTitleScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	WCHAR ch[2] = {};
	if (SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat)
	{
		WCHAR wBuf[BUF_SIZE] = L"";
		if (SceneManager::GetInstance()->m_TitleInfo.pw)
			wcscat(wBuf, SceneManager::GetInstance()->m_TitleInfo.passBuf);
		else
			wcscat(wBuf, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf);

		switch (nMessageID)
		{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
			{
#ifdef WITH_DATABASE
				char pass[NAME_SIZE];
				memset(SceneManager::GetInstance()->m_Name, 0, sizeof(SceneManager::GetInstance()->m_Name));
				WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, -1, SceneManager::GetInstance()->m_Name, NAME_SIZE, NULL, NULL);
				WideCharToMultiByte(CP_UTF8, 0, SceneManager::GetInstance()->m_TitleInfo.passBuf, -1, pass, NAME_SIZE, NULL, NULL);
				NetworkManager::GetInstance()->SendLoginPacket(SceneManager::GetInstance()->m_Name, pass);
#endif
				break;
			}
			case VK_TAB:
				if (SceneManager::GetInstance()->m_TitleInfo.Chat.bOnChat) {
					SceneManager::GetInstance()->m_TitleInfo.pw = !SceneManager::GetInstance()->m_TitleInfo.pw;
				}
				break;
			case VK_BACK: {
				int len = static_cast<int>(wcslen(wBuf));
				if (len <= 0)
					break;
				if (wBuf[len - 1] != 0)
				{
					wBuf[len - 1] = 0;
					if (SceneManager::GetInstance()->m_TitleInfo.pw) {
						_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.passBuf, NULL);
						wcscat(SceneManager::GetInstance()->m_TitleInfo.passBuf, wBuf);
						SceneManager::GetInstance()->m_TitleInfo.passShow[static_cast<int>(wcslen(wBuf))] = true;
					}
					else {
						_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, NULL);
						wcscat(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, wBuf);
					}
				}
				break;
			}
			default:
				break;
			}
			break;
		case WM_CHAR: {
			if (wParam == 0x09 || wParam == 0x0D || wParam == 32 || wParam == 0x08)	//Tab, return, space, backspace
				break;

			//If Buffer is Full
			if (wcsnlen_s(wBuf, BUF_SIZE) > NAME_SIZE - 2) {
				break;
			}
			ch[0] = static_cast<WCHAR>(wParam);
			ch[1] = NULL;
			wcscat(wBuf, ch);
			if (SceneManager::GetInstance()->m_TitleInfo.pw) {
				_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.passBuf, NULL);
				wcscat(SceneManager::GetInstance()->m_TitleInfo.passBuf, wBuf);
				SceneManager::GetInstance()->m_TitleInfo.passShow[static_cast<int>(wcslen(wBuf)) - 2] = false;
			}
			else {
				_wcsset_s(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, NULL);
				wcscat(SceneManager::GetInstance()->m_TitleInfo.Chat.ChatBuf, wBuf);
			}

			break;
		}

		}
	}
	return false;
}

void CTitleScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 1;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(-104.6068f, 5.45f, -241.63f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(-0.3f, -0.85f, -0.3f);
}

void CTitleScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_pd3dGraphicsRootSignature = pd3dGraphicsRootSignature;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 100);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();


	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CTitleScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimated)
{
	//if (m_pUIShader) m_pUIShader->Render(pd3dCommandList, pCamera);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CLobbyScene::CLobbyScene()
{
}

CLobbyScene::~CLobbyScene()
{
}

bool CLobbyScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);

		UILayer::GetInstance()->ProcessMouseClick(SCENEKIND::LOBBY, clickPos);
		CTextureShader::GetInstance()->OnMouseClick();
		break;
	}
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);

		CTextureShader::GetInstance()->Click(2, false);
		CTextureShader::GetInstance()->OnMouseRelease();
		break;
	}
	case WM_MOUSEMOVE:
	{
		POINT clickPos;
		clickPos.x = GET_X_LPARAM(lParam);
		clickPos.y = GET_Y_LPARAM(lParam);
		CTextureShader::GetInstance()->OnMouseMoved(static_cast<float>(clickPos.x), static_cast<float>(clickPos.y));
		break;
	}
	default:
		break;
	}
	return false;
}

bool CLobbyScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	WCHAR ch[2] = {};

	if (SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat)
	{
		switch (nMessageID)
		{
		case WM_IME_COMPOSITION:
		{
			std::wcout.imbue(std::locale("kor"));
			ch[0] = static_cast<WCHAR>(wParam);
			ch[1] = NULL;
			if (lParam & GCS_RESULTSTR)
			{
				wcscat(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, ch);
				SceneManager::GetInstance()->m_LobbyInfo.Chat.TempChatBuf[0] = 0;
			}
			else if (lParam & GCS_COMPSTR)
			{
				wcscpy(SceneManager::GetInstance()->m_LobbyInfo.Chat.TempChatBuf, ch);
			}
			break;
		}
		case WM_CHAR:
			if (wcsnlen_s(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, 256) > CHAT_SIZE - 2) {
				if (wParam == 8) //backspace
				{
					int len = static_cast<int>(wcslen(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf));
					if (len <= 1)
						break;
					if (SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf[len - 2] != 0)
					{
						SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf[len - 2] = 0;
						break;
					}
				}
				else if (wParam == 13) //enter
				{
					SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat = false;
					memset(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, 0, 256);
					//_wcsset_s(m_ChatBuf, NULL);
				}
				break;
			}
			ch[0] = static_cast<WCHAR>(wParam);
			ch[1] = NULL;
			wcscat(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, ch);
			if (wParam == 8) //backspace
			{
				int len = static_cast<int>(wcslen(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf));
				if (len <= 1)
					break;
				if (SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf[len - 2] != 0)
				{
					SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf[len - 2] = 0;
					break;
				}
			}
			else if (wParam == 32)	// space
			{
				ch[0] = 0x20;
				ch[1] = NULL;
				wcscat(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, ch);
			}
			else if (wParam == 13) //enter
			{
				SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat = false;
				memset(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, 0, 256);
			}
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
			{
				NetworkManager::GetInstance()->SendChatPacket(m_pPlayer->GetID(), SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, SceneManager::GetInstance()->m_Name, SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption);

				_wcsset_s(SceneManager::GetInstance()->m_LobbyInfo.Chat.ChatBuf, NULL);
				SceneManager::GetInstance()->m_LobbyInfo.Chat.bOnChat = false;
				break;
			}
			case VK_UP:
			{
				CHAT option = SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption;
				if (option == CHAT::CHANNEL)
					option = CHAT::ALL;
				else
					option = static_cast<CHAT>(static_cast<int>(option) + 1);
				SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption = option;
			}
				
				break;

			case VK_DOWN:
			{
				CHAT option = SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption;
				if (option == CHAT::ALL)
					option = CHAT::CHANNEL;
				else
					option = static_cast<CHAT>(static_cast<int>(option) - 1);
				SceneManager::GetInstance()->m_LobbyInfo.Chat.eChatOption = option;
			}
				break;
			default:
				break;
			}
			break;
		}
	}
	
	switch (nMessageID)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'e':
		case 'E':
		{
			if (m_pNearNpc) m_pNearNpc->Talk();
			break;
		}
		case 'p':
		case 'P':
			NetworkManager::GetInstance()->SendCreateTransactionPacket();
			break;
		}
		break;
	}

	return false;
}

void CLobbyScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = MAX_LIGHTS;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.831f, 0.773f, 0.718f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(-104.6068f, 35.45f, -241.63f);
	//m_pLights[0].m_xmf3Position = XMFLOAT3(-(_PLANE_WIDTH * 0.5f), 512.0f, 0.0f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(-0.5f, -0.7f, -0.5f);
	//m_pLights[0].m_xmf3Direction = XMFLOAT3(+1.0f, -1.0f, 0.0f);

	m_pLights[1].m_bEnable = false;
	m_pLights[1].m_nType = POINT_LIGHT;
	m_pLights[1].m_fRange = 10.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-0.0, 5, -0.0);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(-0.0, -1.0f, -0.0);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);

	m_pLights[2].m_bEnable = false;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[2].m_xmf3Position = XMFLOAT3(-104.6068f, 35.45f, -241.63f);
	m_pLights[2].m_xmf3Direction = XMFLOAT3(-0.3f, -0.85f, -0.3f);

	m_pLights[3].m_bEnable = false;
	m_pLights[3].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(-104.6068f, 35.45f, -241.63f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(-0.3f, -0.85f, -0.3f);
}

void CLobbyScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{	
	m_pd3dGraphicsRootSignature = pd3dGraphicsRootSignature;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 100);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_nHierarchicalGameObjects = 1;
	m_ppHierarchicalGameObjects = new CGameObject * [m_nHierarchicalGameObjects];

	CLoadedModelInfo* pMap = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/LobbyScene_No.bin", NULL);
	m_ppHierarchicalGameObjects[0] = new CMap(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pMap, 0);
	m_ppHierarchicalGameObjects[0]->SetPosition(0.0f, 0.0f, 0.0f);
	m_ppHierarchicalGameObjects[0]->SetScale(1.f, 1.f, 1.f);
	m_ppHierarchicalGameObjects[0]->SetObjectID(1);
	m_ppHierarchicalGameObjects[0]->SetObjectType(OBJ_TYPE::TEXTURE); //Temporary
	m_ppHierarchicalGameObjects[0]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	if (pMap) delete pMap;

	SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToLobbyPercent[LOADING_TEXT::MAP];

	m_ppNpcs = new CLobbyNpc * [LOBBY_NPC];

	CLoadedModelInfo* pShopOwner = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ShopOwner.bin", NULL);
	CLoadedModelInfo* pAuctionOwner = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/AuctionOwner.bin", NULL);
	CLoadedModelInfo* pBlockChainOwner = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/BlockChainOwner.bin", NULL);
	CLoadedModelInfo* pCustomizeOwner = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/CustomizeOwner.bin", NULL);
	m_ppNpcs[0] = new CLobbyNpc(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pShopOwner);
	m_ppNpcs[0]->SetScale(1.2f, 1.2f, 1.2f);
	m_ppNpcs[0]->SetObjectID(5);
	m_ppNpcs[1] = new CLobbyNpc(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pAuctionOwner);
	m_ppNpcs[1]->Rotate(0.0f, 80.0f, 0.0f);
	m_ppNpcs[1]->SetScale(2.5f, 2.5f, 2.5f);
	m_ppNpcs[1]->SetObjectID(6);
	m_ppNpcs[2] = new CLobbyNpc(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pBlockChainOwner);
	m_ppNpcs[2]->Rotate(0.0f, 90.0f, 0.0f);
	m_ppNpcs[2]->SetScale(1.5f, 1.5f, 1.5f);
	m_ppNpcs[2]->SetObjectID(7);
	m_ppNpcs[3] = new CLobbyNpc(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCustomizeOwner);
	m_ppNpcs[3]->Rotate(0.0f, 90.0f, 0.0f);
	m_ppNpcs[3]->SetScale(1.1f, 1.1f, 1.1f);
	m_ppNpcs[3]->SetObjectID(8);

	for (int i = 0; i < LOBBY_NPC; ++i) {
		m_ppNpcs[i]->SetObjectType(OBJ_TYPE::TEXTURE);
		m_ppNpcs[i]->SetPosition(m_npcPositions[i]);
	}

	function<void()> shopCallback = []() {
		CTextureShader::GetInstance()->RandomShopSwitch();
		if (CTextureShader::GetInstance()->IsRandomShop()) {
			SoundManager::GetInstance()->Stop_Sound(CHANNELID::NPC);
			SoundManager::GetInstance()->Play_Sound(L"ShopNpc.mp3", CHANNELID::NPC, 1.2f);
		}
	};
	m_ppNpcs[0]->SetTalkCallback(shopCallback);

	function<void()> auctionCallback = []() {
		CTextureShader::GetInstance()->AuctionSwitch();
		if (CTextureShader::GetInstance()->IsAuction()) {
			NetworkManager::GetInstance()->SendOpenAuctionPacket();
			NetworkManager::GetInstance()->SendGetAuctionInfoPacket(1);
			SoundManager::GetInstance()->Stop_Sound(CHANNELID::NPC);
			SoundManager::GetInstance()->Play_Sound(L"AuctionNpc.mp3", CHANNELID::NPC, 1.2f);
		}
	};
	m_ppNpcs[1]->SetTalkCallback(auctionCallback);

	function<void()> blockChainCallback = []() {
		CTextureShader::GetInstance()->BlockChainSwitch(); 
		if (CTextureShader::GetInstance()->IsBlockchain()) {
			NetworkManager::GetInstance()->SendOpenBlockChainPacket();
			SoundManager::GetInstance()->Stop_Sound(CHANNELID::NPC);
			SoundManager::GetInstance()->Play_Sound(L"BlockChainNpc.wav", CHANNELID::NPC, 1.2f);
		}
	};
	m_ppNpcs[2]->SetTalkCallback(blockChainCallback);

	function<void()> customizeCallback = []() {
		CTextureShader::GetInstance()->CustomizeSwitch();
		if (CTextureShader::GetInstance()->IsCustomize()) {
			NetworkManager::GetInstance()->SendOpenCustomizePacket();
			CS_MOVE_PACKET* p = new CS_MOVE_PACKET;
			p->direction = 0;
			p->move_time = std::chrono::high_resolution_clock::now();
			p->size = sizeof(CS_MOVE_PACKET);
			p->type = CS_MOVE;
			NetworkManager::GetInstance()->SendPacket(p);
			NetworkManager::GetInstance()->myClient->SetDirection(0);
			SoundManager::GetInstance()->Play_Sound(L"CustomizeNpc.wav", CHANNELID::NPC, 1.2f);
		}
	};
	m_ppNpcs[3]->SetTalkCallback(customizeCallback);

	if (pShopOwner) delete pShopOwner;
	if (pAuctionOwner) delete pAuctionOwner;
	if (pBlockChainOwner) delete pBlockChainOwner;
	if (pCustomizeOwner) delete pCustomizeOwner;

	m_BillboardShader = new CBillboardUIShader();
	m_BillboardShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_BillboardShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);
	
	m_nShaders = 0;
	if(m_nShaders)
		m_ppShaders = new CShader * [m_nShaders];	


	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CLobbyScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimated)
{

	//Other Client Update && Render
	for (int i = 0; i < LOBBY_MAX_CLIENT; i++)
	{
		if (m_ppOtherClient[i])
		{
		/*	if (NetworkManager::GetInstance()->otherClientsInfo[i].show && bIsAnimated)
				m_ppOtherClient[i]->Animate(m_fElapsedTime);
			if (!m_ppOtherClient[i]->m_pSkinnedAnimationController) 
				m_ppOtherClient[i]->UpdateTransform(NULL);*/

			if (NetworkManager::GetInstance()->otherClientsInfo[i].show) {
				
				reinterpret_cast<COtherClientPlayer*>(m_ppOtherClient[i])->Update(i);
				reinterpret_cast<CPlayerObject*>(m_ppOtherClient[i])->Customize(NetworkManager::GetInstance()->m_ArrayOtherClientCustom[i]);

				if (bIsAnimated)
					m_ppOtherClient[i]->Animate(m_fElapsedTime);
				if (!m_ppOtherClient[i]->m_pSkinnedAnimationController)
					m_ppOtherClient[i]->UpdateTransform(NULL);

				if(!CTextureShader::GetInstance()->IsCustomize())m_ppOtherClient[i]->Render(pd3dCommandList, pCamera);
			}
		}
	}
	for (int i = 0; i < LOBBY_NPC; ++i) {

		if (m_pNearNpc) {
			m_pNearNpc = m_pNearNpc->CanTalk(m_pPlayer->GetPosition());
		}
		else if (!m_pNearNpc) {
			m_pNearNpc = m_ppNpcs[i]->CanTalk(m_pPlayer->GetPosition());
			if (m_pNearNpc) {
				CTextureShader::GetInstance()->InteractionSwitch(true);
				break;
			}
			else {
				CTextureShader::GetInstance()->InteractionSwitch(false);
				CTextureShader::GetInstance()->AwayFromNpc();
			}
		}
	}

	for (int i = 0; i < LOBBY_NPC; ++i) {
		if (m_ppNpcs[i]) 
		{
			if (bIsAnimated)m_ppNpcs[i]->Animate(m_fElapsedTime);
			if (!m_ppNpcs[i]->m_pSkinnedAnimationController) {
				m_ppNpcs[i]->UpdateTransform(NULL);
			}
			if (!CTextureShader::GetInstance()->IsCustomize())m_ppNpcs[i]->Render(pd3dCommandList, pCamera, m_ppNpcs[i]->iMyShareNum);
		}
	}

	CScene::Render(pd3dCommandList, pCamera, bIsAnimated);
}

void CLobbyScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) 
		m_ppGameObjects[i]->Animate(fTimeElapsed);
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);
	if (CTextureShader::GetInstance()->IsRandomShop())
	{
		if(CTextureShader::GetInstance()->RandomGenParts(fTimeElapsed))CTextureShader::GetInstance()->QuestionIconSwitch();
	}

	/*m_ppParticleObjects[0]->SetForwardVector(m_pPlayer->GetLookVector());
	m_ppParticleObjects[0]->SetPosition(m_pPlayer->GetPosition());*/
	//m_ppParticleObjects[0]->AnimateSprite(m_fElapsedTime);
	//m_ppParticleObjects[0]->Rotate(0, 2, 0); //SpinCenterParticle
}

void CLobbyScene::BillboardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap)
{
	m_BillboardShader->PostRender(pd3dCommandList, pCamera, DescriptorHeap);
}

void CLobbyScene::ReleaseUploadBuffers()
{
	CScene::ReleaseUploadBuffers();
	if (m_BillboardShader)m_BillboardShader->ReleaseUploadBuffers();
}

void CLobbyScene::ReleaseObjects()
{
	CScene::ReleaseObjects();

	if (m_ppNpcs) {
		for (int i = 0; i < LOBBY_NPC; ++i) {
			m_ppNpcs[i]->Release();
		}
		delete[] m_ppNpcs;
	}

	if (m_ppOtherClient)
	{
		for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
			if (m_ppOtherClient[i]) {
				m_ppOtherClient[i]->Release();
				cout << "Other Client Release " << i << endl;
				m_ppOtherClient[i] = nullptr;
			}
		}
		delete[] m_ppOtherClient;
		m_ppOtherClient = nullptr;
	}

	if (m_BillboardShader) m_BillboardShader->ReleaseObjects();
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CReadyScene::CReadyScene()
{
}

CReadyScene::~CReadyScene()
{
}

void CReadyScene::BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel)
{
	for (int i = 0; i < INGAME_PLAYER - 1; ++i) {
		m_ppOtherClient[i] = new COtherClientPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pModel, i);
	}

	for (int i = 0; i < INGAME_PLAYER; ++i) {
		m_ppOtherClient[i]->m_pSkinnedAnimationController->SetTrackEnable(reinterpret_cast<COtherClientPlayer*>(m_ppOtherClient[i])->GetAnimation(), false);
		m_ppOtherClient[i]->m_pSkinnedAnimationController->SetTrackEnable(0, true);
		if (NetworkManager::GetInstance()->GetId() == i) {
			if (i == 3)
				m_ppOtherClient[i]->SetPosition(READY_SCENE_BOSS);
			else
				m_ppOtherClient[i]->SetPosition(XMFLOAT3(-500, -500, -500));
		}
		NetworkManager::GetInstance()->OtherClients[i] = m_ppOtherClient[i];
	}
}

bool CReadyScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);

		UILayer::GetInstance()->ProcessMouseClick(SCENEKIND::READY, clickPos);

		D2D1_RECT_F SkillClickbox = UILayer::GetInstance()->GetSkillClickRect();
		float gapX = SkillClickbox.right - SkillClickbox.left;
		float gapY = SkillClickbox.bottom - SkillClickbox.top;

		ORDER ClientPos = SceneManager::GetInstance()->GetOrder();

		D2D1_RECT_F* PlayerSkillbox = UILayer::GetInstance()->GetPlayerSkillRect(ClientPos);

		int iSkillRenderToJob = static_cast<int>(CTextureShader::GetInstance()->GetPlayerJOB(SceneManager::GetInstance()->GetOrder())) * 12;
		int iSkillRenderToBossJob = static_cast<int>(CTextureShader::GetInstance()->GetBossJob()) * 10;

		if (UILayer::GetInstance()->m_bSkillPopUpClick)
		{
			for (int i = 0; i < 3; ++i)
			{
				for (int j = 0; j < 4; ++j)
				{
					if (ClientPos == ORDER::BOSS && i * 4 + j + 1 > 10) break;
					if (SkillClickbox.left + gapX * j <= clickPos.x && SkillClickbox.right + gapX * j >= clickPos.x && SkillClickbox.top + gapY * i <= clickPos.y && SkillClickbox.bottom + gapY * i >= clickPos.y)
					{
						int num;
						if (ClientPos != ORDER::BOSS)
							num = CTextureShader::GetInstance()->SetSkillArray(iSkillRenderToJob + i * 4 + j, ClientPos);
						else
							num = CTextureShader::GetInstance()->SetSkillArray(iSkillRenderToBossJob + i * 4 + j, ClientPos);
						if (num != -1)
						{
							UILayer::GetInstance()->m_bReadyPlayerSkillRects[static_cast<int>(ClientPos)][num] = false;
							SoundManager::GetInstance()->Play_Sound(L"SkillSelect.wav", CHANNELID::EFFECT);
						}
					}
				}
			}
		}
		for (int i = 0; i < 4; ++i)
		{
			if (!UILayer::GetInstance()->m_bReadyTextShow[static_cast<int>(ClientPos)])
			{
				if (PlayerSkillbox[i].left <= clickPos.x && PlayerSkillbox[i].right >= clickPos.x && PlayerSkillbox[i].top <= clickPos.y && PlayerSkillbox[i].bottom >= clickPos.y)
				{
					CTextureShader::GetInstance()->SetSkillArrayReset(i, ClientPos);
					UILayer::GetInstance()->m_bReadyPlayerSkillRects[static_cast<int>(ClientPos)][i] = true;
					SoundManager::GetInstance()->Play_Sound(L"SkillRemove.wav", CHANNELID::EFFECT);
				}
			}
		}
		CTextureShader::GetInstance()->OnMouseClick();
		break;
	}
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);
		UILayer::GetInstance()->m_bReadyButtonClick = false;
		CTextureShader::GetInstance()->OnMouseRelease();
		break;
	}
	case WM_MOUSEMOVE:
	{
		POINT clickPos;
		clickPos.x = GET_X_LPARAM(lParam);
		clickPos.y = GET_Y_LPARAM(lParam);
		CTextureShader::GetInstance()->OnMouseMoved(static_cast<float>(clickPos.x), static_cast<float>(clickPos.y));
		break;
	}
	default:
		break;
	}
	return false;
}

bool CReadyScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	WCHAR ch[2] = {};

	if (SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat) 
	{
		switch (nMessageID)
		{
		case WM_IME_COMPOSITION:
		{
			std::wcout.imbue(std::locale("kor"));
			ch[0] = static_cast<WCHAR>(wParam);
			ch[1] = NULL;
			if (lParam & GCS_RESULTSTR)
			{
				wcscat(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, ch);
				SceneManager::GetInstance()->m_ReadyInfo.Chat.TempChatBuf[0] = 0;
			}
			else if (lParam & GCS_COMPSTR)
			{
				wcscpy(SceneManager::GetInstance()->m_ReadyInfo.Chat.TempChatBuf, ch);
			}
			break;
		}
		case WM_CHAR:
			if (wcsnlen_s(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, 256) > CHAT_SIZE - 2) {
				if (wParam == 8) //backspace
				{
					int len = static_cast<int>(wcslen(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf));
					if (len <= 1)
						break;
					if (SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf[len - 2] != 0)
					{
						SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf[len - 2] = 0;
						break;
					}
				}
				else if (wParam == 13) //enter
				{
					SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat = false;
					memset(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, 0, 256);
					//_wcsset_s(m_ChatBuf, NULL);
				}
				break;
			}
			ch[0] = static_cast<WCHAR>(wParam);
			ch[1] = NULL;
			wcscat(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, ch);
			if (wParam == 8) //backspace
			{
				int len = static_cast<int>(wcslen(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf));
				if (len <= 1)
					break;
				if (SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf[len - 2] != 0)
				{
					SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf[len - 2] = 0;
					break;
				}
			}
			else if (wParam == 32)	// space
			{
				ch[0] = 0x20;
				ch[1] = NULL;
				wcscat(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, ch);
			}
			else if (wParam == 13) //enter
			{
				SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat = false;
				memset(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, 0, 256);
			}
			break;
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_RETURN:
			{
				NetworkManager::GetInstance()->SendChatPacket(m_pPlayer->GetID(), SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, SceneManager::GetInstance()->m_Name, SceneManager::GetInstance()->m_ReadyInfo.Chat.eChatOption);
				_wcsset_s(SceneManager::GetInstance()->m_ReadyInfo.Chat.ChatBuf, NULL);
				SceneManager::GetInstance()->m_ReadyInfo.Chat.bOnChat = false;
				break;
			}
			case VK_UP:
				break;

			case VK_DOWN:
				break;
			default:
				break;
			}
			break;
		}
	}
	else
	{
		switch (nMessageID)
		{
		case WM_KEYDOWN:
			switch (wParam)
			{
			case VK_LEFT:
			{
				ORDER eClientOrder = SceneManager::GetInstance()->GetOrder();
				if (!UILayer::GetInstance()->m_bReadyTextShow[static_cast<int>(eClientOrder)])
				{
					if (eClientOrder != ORDER::BOSS)
					{
						JOB eJob = CTextureShader::GetInstance()->GetPlayerJOB(eClientOrder);
						if (eJob == JOB::ARCHER)
							eJob = JOB::WIZARD;
						else {
							eJob = static_cast<JOB>(static_cast<int>(eJob) - 1);
						}

						NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(eJob));
						CTextureShader::GetInstance()->SetPlayerJOB(eJob, eClientOrder);
						for (int i = 0; i < 4; ++i)
						{
							CTextureShader::GetInstance()->SetSkillArrayReset(i, eClientOrder);
							UILayer::GetInstance()->m_bReadyPlayerSkillRects[static_cast<int>(eClientOrder)][i] = true;
						}
						dynamic_cast<CGamePlayer*>(m_pPlayer)->SetReadyAnim(static_cast<int>(eJob));
						SceneManager::GetInstance()->SetJob(eJob);

						switch (eJob) {
						case JOB::ARCHER:
							SoundManager::GetInstance()->Play_Sound(L"Archer_StormArrow1.wav", CHANNELID::PLAYER);
							break;
						case JOB::FIGHTER:
							SoundManager::GetInstance()->Play_Sound(L"Fight_Counter.wav", CHANNELID::PLAYER);
							break;
						case JOB::SWORDMAN:
							SoundManager::GetInstance()->Play_Sound(L"Swordman_DefensiveStance.wav", CHANNELID::PLAYER);
							break;
						case JOB::WIZARD:
							SoundManager::GetInstance()->Play_Sound(L"Wizzard_MagicMissile.wav", CHANNELID::PLAYER);
							break;
						}
					}
					else
					{
						if (CTextureShader::GetInstance()->GetBossJob() == BOSSJOB::OGRE)
						{
							CTextureShader::GetInstance()->SetBossJob(BOSSJOB::PROGRAMMER);
							m_pPlayer->DrawOff();
							m_ppOtherClient[INGAME_PLAYER - 1]->DrawOn();
							SoundManager::GetInstance()->Play_Sound(L"Programmer_PlusStat.mp3", CHANNELID::PLAYER);
						}
						else
						{
							CTextureShader::GetInstance()->SetBossJob(BOSSJOB::OGRE);
							m_pPlayer->DrawOn();
							m_ppOtherClient[INGAME_PLAYER - 1]->DrawOff();
							SoundManager::GetInstance()->Play_Sound(L"Orge_Roar.wav", CHANNELID::PLAYER);
						}
							
						
						for (int i = 0; i < 4; ++i)
						{
							CTextureShader::GetInstance()->SetSkillArrayReset(i, eClientOrder);
							UILayer::GetInstance()->m_bReadyPlayerSkillRects[3][i] = true;
						}
						NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(CTextureShader::GetInstance()->GetBossJob()) + MAX_JOB);
					}
				}
			}
			break;
			case VK_RIGHT:
			{
				ORDER eClientOrder = SceneManager::GetInstance()->GetOrder();
				if (!UILayer::GetInstance()->m_bReadyTextShow[static_cast<int>(eClientOrder)])
				{
					if (eClientOrder != ORDER::BOSS)
					{
						JOB eJob = CTextureShader::GetInstance()->GetPlayerJOB(eClientOrder);
						eJob = static_cast<JOB>(static_cast<int>(eJob) + 1);
						if (eJob == JOB::NONE)
							eJob = JOB::ARCHER;

						NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(eJob));
						CTextureShader::GetInstance()->SetPlayerJOB(eJob, eClientOrder);
						for (int i = 0; i < 4; ++i)
						{
							CTextureShader::GetInstance()->SetSkillArrayReset(i, eClientOrder);
							UILayer::GetInstance()->m_bReadyPlayerSkillRects[static_cast<int>(eClientOrder)][i] = true;
						}
						dynamic_cast<CGamePlayer*>(m_pPlayer)->SetReadyAnim(static_cast<int>(eJob));
						SceneManager::GetInstance()->SetJob(eJob);

						switch (eJob) {
						case JOB::ARCHER:
							SoundManager::GetInstance()->Play_Sound(L"Archer_StormArrow1.wav", CHANNELID::PLAYER);
							break;
						case JOB::FIGHTER:
							SoundManager::GetInstance()->Play_Sound(L"Fight_Counter.wav", CHANNELID::PLAYER);
							break;
						case JOB::SWORDMAN:
							SoundManager::GetInstance()->Play_Sound(L"Swordman_DefensiveStance.wav", CHANNELID::PLAYER);
							break;
						case JOB::WIZARD:
							SoundManager::GetInstance()->Play_Sound(L"Wizzard_MagicMissile.wav", CHANNELID::PLAYER);
							break;
						}
					}
					else
					{
						if (CTextureShader::GetInstance()->GetBossJob() == BOSSJOB::OGRE)
						{
							CTextureShader::GetInstance()->SetBossJob(BOSSJOB::PROGRAMMER);
							m_pPlayer->DrawOff();
							m_ppOtherClient[INGAME_PLAYER - 1]->DrawOn();
							SoundManager::GetInstance()->Play_Sound(L"Programmer_PlusStat.mp3", CHANNELID::PLAYER);
						}
						else
						{
							CTextureShader::GetInstance()->SetBossJob(BOSSJOB::OGRE);
							m_pPlayer->DrawOn();
							m_ppOtherClient[INGAME_PLAYER - 1]->DrawOff();
							SoundManager::GetInstance()->Play_Sound(L"Orge_Roar.wav", CHANNELID::PLAYER);
						}
							

						for (int i = 0; i < 4; ++i)
						{
							CTextureShader::GetInstance()->SetSkillArrayReset(i, eClientOrder);
							UILayer::GetInstance()->m_bReadyPlayerSkillRects[3][i] = true;
						}
						NetworkManager::GetInstance()->SendJobSelectPacket(static_cast<int>(CTextureShader::GetInstance()->GetBossJob()) + MAX_JOB);
					}
				}
			}
			break;
			default:
				break;
			}
			break;
		default:
			break;
		}
	}
	return false;
}

void CReadyScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 1;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_pLights[0].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(-104.6068f, 5.45f, -241.63f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(-0.5f, -0.7f, -0.5f);
}

void CReadyScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
	m_pd3dGraphicsRootSignature = pd3dGraphicsRootSignature;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 120);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	m_nHierarchicalGameObjects = 1;
	m_ppHierarchicalGameObjects = new CGameObject * [m_nHierarchicalGameObjects];

	CLoadedModelInfo* pMap = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/LobbyScene_No.bin", NULL);
	m_ppHierarchicalGameObjects[0] = new CMap(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pMap, 0);
	m_ppHierarchicalGameObjects[0]->SetPosition(0.0f, 0.0f, 0.0f);
	m_ppHierarchicalGameObjects[0]->SetScale(1.f, 1.f, 1.f);
	m_ppHierarchicalGameObjects[0]->SetObjectID(1);
	m_ppHierarchicalGameObjects[0]->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	if (pMap) delete pMap;

	SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToReadyPercent[LOADING_TEXT::MAP];

	m_ppOtherClient = new CGameObject * [INGAME_PLAYER];
	CLoadedModelInfo* pBoss = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Boss_Programmer.bin", NULL);
	m_ppOtherClient[INGAME_PLAYER - 1] = new COtherClientPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pBoss, INGAME_PLAYER - 1);
	m_ppOtherClient[INGAME_PLAYER - 1]->DrawOff();

	if (pBoss)
		delete pBoss;
	//m_ppOtherClient[0]->SetScale(1.f, 1.f, 1.f);

	///*
	/*m_nShaders = 0;
	m_ppShaders = new CShader * [m_nShaders];*/

	SceneManager::GetInstance()->m_fLoadingProgressPercent = 0.6f;


	CreateShaderVariables(pd3dDevice, pd3dCommandList);
}

void CReadyScene::ReleaseObjects()
{
	CScene::ReleaseObjects();

	if (m_ppOtherClient)
	{
		for (int i = 0; i < INGAME_PLAYER; i++) 
			if (m_ppOtherClient[i]) 
				m_ppOtherClient[i]->Release();
		delete[] m_ppOtherClient;
	}
}

void CReadyScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;

	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->Animate(fTimeElapsed);
	for (int i = 0; i < m_nShaders; i++) if (m_ppShaders[i]) m_ppShaders[i]->AnimateObjects(fTimeElapsed);
}

void CReadyScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimated)
{
	CScene::Render(pd3dCommandList, pCamera, bIsAnimated);

	for (int i = 0; i < INGAME_PLAYER; ++i)
	{
		if (m_ppOtherClient[i])
		{
			if (m_ppOtherClient[i] && bIsAnimated)
				m_ppOtherClient[i]->Animate(m_fElapsedTime);
			if (!m_ppOtherClient[i]->m_pSkinnedAnimationController)
				m_ppOtherClient[i]->UpdateTransform(NULL);

			if (NetworkManager::GetInstance()->otherClientsInfo[i].show == true) {
				reinterpret_cast<COtherClientPlayer*>(m_ppOtherClient[i])->Update(i);
				if ((i == NetworkManager::GetInstance()->GetId() && SceneManager::GetInstance()->GetOrder() == ORDER::BOSS) || i == 3) {
				}
				else {
					reinterpret_cast<CPlayerObject*>(m_ppOtherClient[i])->Customize(NetworkManager::GetInstance()->m_ArrayInGameClientsCustom[i]);
					reinterpret_cast<CPlayerObject*>(m_ppOtherClient[i])->SetWeapon(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[i]));
				}
				m_ppOtherClient[i]->Render(pd3dCommandList, pCamera);
			}
		}
	}
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//

CIngameScene::CIngameScene()
{
}

CIngameScene::~CIngameScene()
{
}

void CIngameScene::ReleaseObjects()
{
	CScene::ReleaseObjects();

	if (m_minions) {
		for (int i = 0; i < MAX_MINION; ++i) {
			m_minions[i]->Release();
		}
		delete[] m_minions;
		m_minions = nullptr;
	}

	if (m_monsters) {
		for (int i = 0; i < MONSTER_NUM; ++i) {
			m_monsters[i]->Release();
		}
		delete[] m_monsters;
		m_monsters = nullptr;
	}

	if (m_towerAttacks) {
		for (int i = 0; i < PATH_NUM; ++i) {
			m_towerAttacks[i]->Release();
		}
		delete[] m_towerAttacks;
		m_towerAttacks = nullptr;
	}
	if (m_BillboardShader) m_BillboardShader->ReleaseObjects();

	for (auto p : m_skillObjects)
	{
		for (auto q : p.second)
		{
			if (q)
			{
				delete q;
				q = nullptr;
			}
		}
		p.second.clear();
	}
	m_skillObjects.clear();

	for (auto& pair : m_ParticleObjects)
	{
		for (auto& vector : pair.second)
		{
			for (auto pParticleObject : vector)
			{
				if (pParticleObject)
				{
					delete pParticleObject;
					pParticleObject = nullptr;
				}
			}
			vector.clear();
		}
	}
	m_ParticleObjects.clear();
	
	for (auto& pair : m_SkillTypeParticle)
	{
		for (auto pParticleObject : pair.second)
		{
			if (pParticleObject)
			{
				delete pParticleObject;
				pParticleObject = nullptr;
			}
		}
		pair.second.clear();
	}
	m_SkillTypeParticle.clear();

	if (m_pParticleTexture) 
	{
		for (int i = 0; i < m_iParticleTextureNum; i++) 
		{
			m_pParticleTexture[i]->Release();
		}
		delete[] m_pParticleTexture;
		m_pParticleTexture = nullptr;
	}


	for (auto& pair : SceneManager::GetInstance()->m_ParticleInfo)
	{
		for (auto& vector : pair.second)
		{
			for (auto pParticleObject : vector)
			{
				if (pParticleObject)
				{
					delete pParticleObject;
					pParticleObject = nullptr;
				}
			}
			vector.clear();
		}
	}
	SceneManager::GetInstance()->m_ParticleInfo.clear();
}

void CIngameScene::BuildOtherClient(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, CLoadedModelInfo* pModel)
{
	for (int i = 0; i < INGAME_PLAYER - 1; ++i) {
		m_ppOtherClient[i] = new COtherClientPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pModel, i);
		NetworkManager::GetInstance()->OtherClients[i] = m_ppOtherClient[i];
	}
}

bool CIngameScene::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID)
	{
	case WM_LBUTTONDOWN:
	{
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);

		reinterpret_cast<CGamePlayer*>(m_pPlayer)->UseSkillCheck(SKILLKIND::LEFTCLICK);
		CTextureShader::GetInstance()->OnMouseClick();
		break;
	}
	case WM_RBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&(SceneManager::GetInstance()->ptCursorPos));
		reinterpret_cast<CGamePlayer*>(m_pPlayer)->UseSkillCheck(SKILLKIND::RIGHTCLICK);
		break;
	case WM_LBUTTONUP:
	case WM_RBUTTONUP:
	{
		::ReleaseCapture();
		POINT clickPos = SceneManager::GetInstance()->ptCursorPos;
		ScreenToClient(hWnd, &clickPos);
		CTextureShader::GetInstance()->OnMouseRelease();
		break;
	}
	case WM_MOUSEMOVE:
	{
		POINT clickPos;
		clickPos.x = GET_X_LPARAM(lParam);
		clickPos.y = GET_Y_LPARAM(lParam);
		CTextureShader::GetInstance()->OnMouseMoved(static_cast<float>(clickPos.x), static_cast<float>(clickPos.y));
		break;
	}
	default:
		break;
	}
	return false;
}

bool CIngameScene::OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam)
{
	switch (nMessageID) {
	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'l':
		case 'L':
			NetworkManager::GetInstance()->SendDebugGoldPacket();
			break;
		case VK_LEFT:
			m_pPlayer->Rotate(0.f, -10.0f, 0.f);
			break;
		case VK_RIGHT:
			m_pPlayer->Rotate(0.f, 10.0f, 0.f);
			break;
		case VK_SPACE:
		{
			NetworkManager::GetInstance()->SendJumpPacket();

			XMFLOAT3 JUMP_START_POS[6] = { XMFLOAT3(-7.46f, 0.f, -79.55f), XMFLOAT3(-40.3f, 0.f, -48.82f), XMFLOAT3(-66.6f, 0.f, -9.7f), XMFLOAT3(-113.7f, 0.f, -52.6f), XMFLOAT3(-79.69f, 0.f, -80.74f), XMFLOAT3(-46.88f, 0.f, -118.05f) };
			for (int i = 0; i < 6; ++i) {
				if (::sqrt(::pow(JUMP_START_POS[i].x - m_pPlayer->GetPosition().x, 2) + ::pow(JUMP_START_POS[i].z - m_pPlayer->GetPosition().z, 2)) < 2.f) {
					SoundManager::GetInstance()->Play_Sound(L"Jump.wav", CHANNELID::EFFECT);
					reinterpret_cast<CGamePlayer*>(m_pPlayer)->SetJumping(true);


					break;
				}
			}
			break;
		}
		case 'e':
		case 'E':
		{
			XMFLOAT3 pos = m_pPlayer->GetPosition();

			//Check Teleport Distance
			for (int i = 0; i < TELEPORT_POS.size(); ++i) {
				if (::sqrt(::pow(TELEPORT_POS[i].x - pos.x, 2) + ::pow(TELEPORT_POS[i].y - pos.z, 2)) < TELEPORT_INTERACTION_DISTANCE) {
					NetworkManager::GetInstance()->SendTeleportPacket();
				}
			}

			//Checkt Tower Distance
			for (int i = 0; i < TOWER_POS.size(); ++i) {
				if (::sqrt(::pow(TOWER_POS[i].x - pos.x, 2) + ::pow(TOWER_POS[i].z - pos.z, 2)) < TOWER_INTERACTION_DISTANCE) {
					NetworkManager::GetInstance()->SendTowerActivatePacket(i);
					break;
				}
			}
			
		}
			break;
		case 'Q':
			reinterpret_cast<CGamePlayer*>(m_pPlayer)->UseSkillCheck(SKILLKIND::Q);
			break;
		case 'R':
			reinterpret_cast<CGamePlayer*>(m_pPlayer)->UseSkillCheck(SKILLKIND::R);
			break;
		case VK_SHIFT:
			reinterpret_cast<CGamePlayer*>(m_pPlayer)->UseSkillCheck(SKILLKIND::SHIFT);
			break;
		case 'z':
		case 'Z':
			NetworkManager::GetInstance()->SendMinionPathPacket(0);
			break;
		case 'x':
		case 'X':
			NetworkManager::GetInstance()->SendMinionPathPacket(1);
			break;
		case 'c':
		case 'C':
			NetworkManager::GetInstance()->SendMinionPathPacket(2);
			break;
		case 'v':
		case 'V':
			NetworkManager::GetInstance()->SendMinionPathPacket(3);
			break;

		case 'm':
		case 'M':
			CTextureShader::GetInstance()->MinimapSwitch();
			break;

		case 'p':
		case 'P':
			if (NetworkManager::GetInstance()->GetId() != 3 && sqrtf(powf(SHOP_POS_X - m_pPlayer->GetPosition().x, 2.f) + powf(SHOP_POS_Z - m_pPlayer->GetPosition().z, 2.f)) < SHOP_DISTANCE) {
				CTextureShader::GetInstance()->ShopSwitch();
				SoundManager::GetInstance()->Play_Sound(L"ShopOpen.wav", CHANNELID::EFFECT);
			}
			else if (NetworkManager::GetInstance()->GetId() == 3 && sqrtf(powf(BOSS_SHOP_POS_X - m_pPlayer->GetPosition().x, 2.f) + powf(BOSS_SHOP_POS_Z - m_pPlayer->GetPosition().z, 2.f)) < SHOP_DISTANCE) {
				CTextureShader::GetInstance()->ShopSwitch();
				SoundManager::GetInstance()->Play_Sound(L"ShopOpen.wav", CHANNELID::EFFECT);				
			}

			break;

		case '1': dynamic_cast<CGamePlayer*>(m_pPlayer)->UseItem(0); break;
		case '2': dynamic_cast<CGamePlayer*>(m_pPlayer)->UseItem(1); break;
		case '3': dynamic_cast<CGamePlayer*>(m_pPlayer)->UseItem(2); break;
		case '4': dynamic_cast<CGamePlayer*>(m_pPlayer)->UseItem(3); break;
		case '5': dynamic_cast<CGamePlayer*>(m_pPlayer)->UseItem(4); break;
		}
	}
	return false;
}

void CIngameScene::BuildDefaultLightsAndMaterials()
{
	m_nLights = 4;
	m_pLights = new LIGHT[m_nLights];
	::ZeroMemory(m_pLights, sizeof(LIGHT) * m_nLights);

	m_xmf4GlobalAmbient = XMFLOAT4(0.15f, 0.15f, 0.15f, 1.0f);

	m_pLights[0].m_bEnable = true;
	m_pLights[0].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[0].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	//m_pLight0[2].m_xmf4Diffuse = XMFLOAT4(0.831f, 0.773f, 0.718f, 1.2f); // 밝은 빛
	m_pLights[0].m_xmf4Diffuse = XMFLOAT4(0.95f, 0.8f, 1.0f, 1.2f); // 연보랏빛
	//m_pLight0[2].m_xmf4Diffuse = XMFLOAT4(0.8f, 0.3f, 1.0f, 1.2f); // 보랏빛
	m_pLights[0].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[0].m_xmf3Position = XMFLOAT3(-104.6068f, 5.45f, -241.63f);
	m_pLights[0].m_xmf3Direction = XMFLOAT3(-0.5f, -0.7f, -0.5f);

	m_pLights[1].m_bEnable = false;
	m_pLights[1].m_nType = POINT_LIGHT;
	m_pLights[1].m_fRange = 3.0f;
	m_pLights[1].m_xmf4Ambient = XMFLOAT4(0.831f, 0.773f, 0.718f, 0.0f);
	m_pLights[1].m_xmf4Diffuse = XMFLOAT4(0.0f, 0.0f, 0.5f, 1.0f);
	m_pLights[1].m_xmf4Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pLights[1].m_xmf3Position = XMFLOAT3(-0.0, 5, -0.0);
	m_pLights[1].m_xmf3Direction = XMFLOAT3(-0.0, -1.0f, -0.0);
	m_pLights[1].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);

	m_pLights[2].m_bEnable = false;
	m_pLights[2].m_nType = DIRECTIONAL_LIGHT;
	m_pLights[2].m_xmf4Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_pLights[2].m_xmf4Diffuse = XMFLOAT4(0.4f, 0.3f, 0.8f, 1.0f);
	m_pLights[2].m_xmf4Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 0.0f);
	m_pLights[2].m_xmf3Position = XMFLOAT3(230.0f, 330.0f, 480.0f);

	m_pLights[3].m_bEnable = false;
	m_pLights[3].m_nType = SPOT_LIGHT;
	m_pLights[3].m_fRange = 600.0f;
	m_pLights[3].m_xmf4Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pLights[3].m_xmf4Diffuse = XMFLOAT4(0.3f, 0.7f, 0.0f, 1.0f);
	m_pLights[3].m_xmf4Specular = XMFLOAT4(0.3f, 0.3f, 0.3f, 0.0f);
	m_pLights[3].m_xmf3Position = XMFLOAT3(550.0f, 330.0f, 530.0f);
	m_pLights[3].m_xmf3Direction = XMFLOAT3(0.0f, -1.0f, 1.0f);
	m_pLights[3].m_xmf3Attenuation = XMFLOAT3(1.0f, 0.01f, 0.0001f);
	m_pLights[3].m_fFalloff = 8.0f;
	m_pLights[3].m_fPhi = (float)cos(XMConvertToRadians(90.0f));
	m_pLights[3].m_fTheta = (float)cos(XMConvertToRadians(30.0f));

}

#define MAP_BUILDING

void CIngameScene::BuildObjects(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature)
{
#ifdef Test
	cout << "Ingame Initialize Start" << endl;
#endif // TEST

	m_pd3dGraphicsRootSignature = pd3dGraphicsRootSignature;

	CreateCbvSrvDescriptorHeaps(pd3dDevice, 0, 200);

	CMaterial::PrepareShaders(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);

	BuildDefaultLightsAndMaterials();	

	m_pSkyBox = new CSkyBox(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
#ifdef MAP_BUILDING	
	m_nHierarchicalGameObjects = 1;
	m_ppHierarchicalGameObjects = new CGameObject * [m_nHierarchicalGameObjects];

	//For Test
	CLoadedModelInfo* pMap = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Plane1.bin", NULL);
	m_ppHierarchicalGameObjects[0] = new CMap(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pMap, 0);
	m_ppHierarchicalGameObjects[0]->SetPosition(0.0f, 0.0f, 0.0f);
	m_ppHierarchicalGameObjects[0]->SetScale(1.f, 1.f, 1.f);
	m_ppHierarchicalGameObjects[0]->CreateShaderVariables(pd3dDevice, pd3dCommandList);

	if (m_ppHierarchicalGameObjects[0]->FindFrame("SM_platform_moba")) m_ppHierarchicalGameObjects[0]->FindFrame("SM_platform_moba")->m_fMaxRadius *= 1.5f;
	if (m_ppHierarchicalGameObjects[0]->FindFrame("SM_platform_moba_(1)")) m_ppHierarchicalGameObjects[0]->FindFrame("SM_platform_moba_(1)")->m_fMaxRadius *= 1.5f;

	if (pMap) delete pMap;


	SceneManager::GetInstance()->m_fLoadingProgressPercent = SceneManager::GetInstance()->ToIngamePercent[LOADING_TEXT::MAP];
#else
	m_nHierarchicalGameObjects = 0/*1*/;
	m_ppHierarchicalGameObjects = new CGameObject * [m_nHierarchicalGameObjects];
	///*
#endif
	m_minions = new CGameObject * [MAX_MINION];
	CLoadedModelInfo* pMinion = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/FreeLichPBR.bin", NULL);
	for (int i = 0; i < MAX_MINION; ++i) {
		m_minions[i] = new CMinion(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pMinion, 1);
		m_minions[i]->SetScale(MINION_SCALE, MINION_SCALE, MINION_SCALE);
		NetworkManager::GetInstance()->minions[i] = reinterpret_cast<CMinion*>(m_minions[i]);
		m_minions[i]->SetObjectID(2);
		m_minions[i]->SetObjectType(OBJ_TYPE::TEXTURE);
	}
	
	if (pMinion) delete pMinion;

	m_monsters = new CGameObject * [MONSTER_NUM];
	CLoadedModelInfo* pUnique = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Unique/Red.bin", NULL);
	CLoadedModelInfo* pRare1 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Rare/Green.bin", NULL);
	CLoadedModelInfo* pRare2 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Rare/PBR_Golem.bin", NULL);
	CLoadedModelInfo* pNormal1 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Normal/Bear_4.bin", NULL);
	CLoadedModelInfo* pNormal2 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Normal/minotaur1.bin", NULL);
	CLoadedModelInfo* pNormal3 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Normal/ChestMonsterPBR.bin", NULL);
	CLoadedModelInfo* pNormal4 = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Monsters/Normal/BeholderPBR.bin", NULL);
	m_monsters[0] = new CUniqueRed(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pUnique);
	m_monsters[0]->SetScale(UNIQUE_RED_SCALE, UNIQUE_RED_SCALE, UNIQUE_RED_SCALE);
	m_monsters[1] = new CRareGreen(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pRare1);
	//m_monsters[1]->SetScale(RARE_GREEN_SCALE, RARE_GREEN_SCALE, RARE_GREEN_SCALE);
	m_monsters[2] = new CRareGolem(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pRare2);
	m_monsters[2]->SetScale(RARE_GOLEM_SCALE, RARE_GOLEM_SCALE, RARE_GOLEM_SCALE);
	m_monsters[3] = new CNormalBear(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal1);
	m_monsters[3]->SetScale(NORMAL_BEAR_SCALE, NORMAL_BEAR_SCALE, NORMAL_BEAR_SCALE);
	m_monsters[4] = new CNormalMinotaur(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal2);
	m_monsters[4]->SetScale(NORMAL_MINOTAUR_SCALE, NORMAL_MINOTAUR_SCALE, NORMAL_MINOTAUR_SCALE);
	m_monsters[5] = new CNormalChest(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal3);
	m_monsters[5]->SetScale(NORMAL_CHEST_SCALE, NORMAL_CHEST_SCALE, NORMAL_CHEST_SCALE);
	m_monsters[6] = new CNormalBeholder(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal4);
	m_monsters[6]->SetScale(NORMAL_BEHOLDER_SCALE, NORMAL_BEHOLDER_SCALE, NORMAL_BEHOLDER_SCALE);
	m_monsters[7] = new CNormalChest(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal3);
	m_monsters[7]->SetScale(NORMAL_CHEST_SCALE, NORMAL_CHEST_SCALE, NORMAL_CHEST_SCALE);
	m_monsters[8] = new CNormalBeholder(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pNormal4);
	m_monsters[8]->SetScale(NORMAL_BEHOLDER_SCALE, NORMAL_BEHOLDER_SCALE, NORMAL_BEHOLDER_SCALE);


	for (int i = 0; i < MONSTER_NUM; ++i) {
		NetworkManager::GetInstance()->monsters[i] = reinterpret_cast<CMonster*>(m_monsters[i]);
		m_monsters[i]->SetObjectID(3);
		m_monsters[i]->SetObjectType(OBJ_TYPE::TEXTURE);
	}

	if (pUnique) delete pUnique;
	if (pRare1) delete pRare1;
	if (pRare2) delete pRare2;
	if (pNormal1) delete pNormal1;
	if (pNormal2) delete pNormal2;
	if (pNormal3) delete pNormal3;
	if (pNormal4) delete pNormal4;

	m_ppOtherClient = new CGameObject * [INGAME_PLAYER];

	CLoadedModelInfo* pBoss = nullptr;
	if (NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::OGRE))
		pBoss = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Boss_Ogre.bin", NULL);
	else if(NetworkManager::GetInstance()->readySceneInfo->playerJobs[3] - MAX_JOB == static_cast<int>(BOSSJOB::PROGRAMMER))
		pBoss = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Boss_Programmer.bin", NULL);

	m_ppOtherClient[INGAME_PLAYER - 1] = new COtherClientPlayer(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pBoss, INGAME_PLAYER - 1);
	NetworkManager::GetInstance()->OtherClients[INGAME_PLAYER - 1] = m_ppOtherClient[INGAME_PLAYER - 1];

	if (pBoss) delete pBoss;

	m_towerAttacks = new CGameObject * [PATH_NUM];
	CLoadedModelInfo* pCube = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Cube.bin", NULL);
	for (int i = 0; i < PATH_NUM; ++i) {
		m_towerAttacks[i] = new CTowerAttack(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, pCube);
		NetworkManager::GetInstance()->towerAttacks[i] = reinterpret_cast<CTowerAttack*>(m_towerAttacks[i]);
	}
	if (pCube)
		delete pCube;

	if(CSkillModel::pWizardModel == nullptr)
		CSkillModel::pWizardModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Cube.bin", NULL);
	if(CSkillModel::pArcherModel == nullptr)
		CSkillModel::pArcherModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ArrowModel.bin", NULL);
	if(CSkillModel::pOgreModel == nullptr)
		CSkillModel::pOgreModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/SM_rock_001.bin", NULL);
	if(CSkillModel::pAreaHelloWorldModel == nullptr)
		CSkillModel::pAreaHelloWorldModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/Helloworld.bin", NULL);
	if(CSkillModel::pProtectedAreaModel == nullptr)
		CSkillModel::pProtectedAreaModel = CGameObject::LoadGeometryAndAnimationFromFile(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, "Model/ProtectedArea.bin", NULL);

	CBlendSkillShader* pSkillShader = new CBlendSkillShader();
	pSkillShader->CreateShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, 0);
	pSkillShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pSkillShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 3);

	m_BillboardShader = new CBillboardUIShader(m_monsters, m_minions, m_ppOtherClient);
	m_BillboardShader->CreateShader(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature);
	m_BillboardShader->BuildObjects(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, NULL);


	//Skill Objects
	for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
		m_skillObjects[SKILL_TYPE::WIZARD_ATTACK].push_back(new WizardAttack(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::WIZARD_MAGIC_MISSILE].push_back(new MagicMissile(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::WIZARD_ENERGY_BALL].push_back(new EnergyBall(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::WIZARD_BIGBANG].push_back(new BigBang(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::WIZARD_BIGBANG_CONTINUE].push_back(new BigBang(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::WIZARD_DARKNESS_RAY].push_back(new DarknessRay(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::SWORDMAN_AURA_BLADE].push_back(new AuraBlade(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD].push_back(new JudgementSword(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::SWORDMAN_PROTECTED_AREA].push_back(new ProtectedArea(pd3dDevice, pd3dCommandList, CSkillModel::pProtectedAreaModel, pSkillShader));
		m_skillObjects[SKILL_TYPE::ARCHER_ATTACK].push_back(new ArcherAttack(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::ARCHER_PHOENIX_ARROW].push_back(new PhoenixArrow(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::ARCHER_PENETRAITING_SHOT].push_back(new PenetraitingShot(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::ARCHER_STICKY_ARROW].push_back(new StickyArrow(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::ARCHER_STROM_ARROW].push_back(new ArcherObject(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::ARCHER_ARROW_RAIN].push_back(new ArcherObject(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
		m_skillObjects[SKILL_TYPE::FIGHTER_FIREBALL].push_back(new FireBall(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::PRO_RETURN_ZERO].push_back(new ReturnZero(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::PRO_SCL].push_back(new SCL(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::PRO_ATTACK].push_back(new ProAttack(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::PRO_HELLO_WORLD].push_back(new HelloWorld(pd3dDevice, pd3dCommandList, CSkillModel::pAreaHelloWorldModel, pSkillShader));
		m_skillObjects[SKILL_TYPE::OGRE_ROCK_THROW].push_back(new RockThrow(pd3dDevice, pd3dCommandList, CSkillModel::pOgreModel));
		m_skillObjects[SKILL_TYPE::OGRE_DIMENSION_CRUSH].push_back(new DimensionCrush(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
		m_skillObjects[SKILL_TYPE::OGRE_DIMENSION_PUNCH].push_back(new CSkillObject(pd3dDevice, pd3dCommandList, CSkillModel::pWizardModel));
	}

	for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
		m_skillObjects[SKILL_TYPE::ARCHER_MULTIPLE_SHOT].push_back(new MultipleShot(pd3dDevice, pd3dCommandList, CSkillModel::pArcherModel));
	}

	//Create Particle Shared Texture 
	m_iParticleTextureNum = PARTICLE_ADDRESS::ADDRESS_COUNT;
	m_pParticleTexture = new CTexture* [m_iParticleTextureNum];

	for (int i = 0; i < m_iParticleTextureNum; ++i)
	{
		m_pParticleTexture[i] = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
		m_pParticleTexture[i]->LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, ParticleTextureAddress[i], RESOURCE_TEXTURE2D, 0);
		CreateShaderResourceViews(pd3dDevice, m_pParticleTexture[i], 0, 21);
	}

	XMFLOAT4* pxmf4RandomValues = new XMFLOAT4[1024];
	for (int i = 0; i < 1024; i++) { pxmf4RandomValues[i].x = float((Util::GenerateRandomInt(0, RAND_MAX) % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].y = float((Util::GenerateRandomInt(0, RAND_MAX) % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].z = float((Util::GenerateRandomInt(0, RAND_MAX) % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].w = float((Util::GenerateRandomInt(0, RAND_MAX) % 10000) - 5000) / 5000.0f; }

	//	m_pRandowmValueTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);
	CTexture* pRandowmValueTexture = new CTexture(1, RESOURCE_BUFFER, 0, 1);
	pRandowmValueTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 1024, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	CTexture*  pRandowmValueOnSphereTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);
	pRandowmValueOnSphereTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 256, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	CreateShaderResourceViews(pd3dDevice, pRandowmValueTexture, 0, 22);
	CreateShaderResourceViews(pd3dDevice, pRandowmValueOnSphereTexture, 0, 23);

	CParticleShader* pShader = new CParticleShader();
	pShader->CreateParticleShader(pd3dDevice, pd3dCommandList, pd3dGraphicsRootSignature, 0);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 3);


	for (int j = 0; j < MAX_SKILL_OBJECT; ++j)
	{
		for (int i = 0; i < static_cast<int>(SKILL_TYPE::TYPE_COUNT); ++i)
		{
			int numParticle = PARTICLE_SKILLSETTING::NumParticle(static_cast<SKILL_TYPE>(i));
			for (int k = 0; k < numParticle; ++k)
			{
				int Type = PARTICLE_SKILLSETTING::Type(static_cast<SKILL_TYPE>(i), k);
				int pszFileName = PARTICLE_SKILLSETTING::TextureAddress(static_cast<SKILL_TYPE>(i), k);
				if (pszFileName != PARTICLE_ADDRESS::ADDRESS_COUNT && Type != PARTICLE_TYPE::NONE && numParticle != 0)
				{
					CParticleObject* newParticle = new CParticleObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pParticleTexture[pszFileName], pRandowmValueTexture, pRandowmValueOnSphereTexture, pShader, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), MAX_PARTICLES, Type);
					newParticle->SettingDetail(static_cast<SKILL_TYPE>(i), k);
					m_SkillTypeParticle[static_cast<SKILL_TYPE>(i)].push_back(newParticle);
				}
			}
		}
	}
	

	ParticleInfo* pInfo;
	CParticleObject* pObj;
	int Type = PARTICLE_TYPE::NONE;
	int numParticle = 0;
	int pszFileName = PARTICLE_ADDRESS::ADDRESS_COUNT;
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			numParticle = PARTICLE_SKILLSETTING::NumParticle(NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j]);
			for (int k = 0; k < numParticle; ++k)
			{
				Type = PARTICLE_SKILLSETTING::Type(NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j], k);
				pszFileName = PARTICLE_SKILLSETTING::TextureAddress(NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j], k);

				if (pszFileName != PARTICLE_ADDRESS::ADDRESS_COUNT && Type != PARTICLE_TYPE::NONE && numParticle != 0)
				{
					pObj = new CParticleObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pParticleTexture[pszFileName], pRandowmValueTexture, pRandowmValueOnSphereTexture, pShader, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), MAX_PARTICLES, Type);
					pInfo = new ParticleInfo();
					//Particle detail setting
					pObj->SettingDetail(static_cast<PARTICLE_SITUATION>(i), NetworkManager::GetInstance()->readySceneInfo->selectSkills[i][j]);
					//
					m_ParticleObjects[static_cast<PARTICLE_SITUATION>(i)][j + 1].push_back(pObj);
					SceneManager::GetInstance()->m_ParticleInfo[static_cast<PARTICLE_SITUATION>(i)][j + 1].push_back(pInfo);
				}
			}
		}
	}


	//jump particle
	for (int i = 0; i < 6; ++i)
	{
		pObj = new CParticleObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pParticleTexture[PARTICLE_ADDRESS::SPRITEJUMP], pRandowmValueTexture, pRandowmValueOnSphereTexture, pShader, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), MAX_PARTICLES, PARTICLE_TYPE::JUMPEFEECT);
		//Particle detail setting
		XMFLOAT3 JUMP_START_POS[6] = { XMFLOAT3(-7.46f, 0.f, -79.55f), XMFLOAT3(-40.3f, 0.f, -48.82f), XMFLOAT3(-66.6f, 0.f, -9.7f), XMFLOAT3(-113.7f, 0.f, -52.6f), XMFLOAT3(-79.69f, 0.f, -80.74f), XMFLOAT3(-46.88f, 0.f, -118.05f) };
		pInfo = new ParticleInfo();
		pInfo->pos = JUMP_START_POS[i];
		pInfo->show = true;

		pObj->SettingDetail(PARTICLE_SITUATION::JUMP, i);
		pObj->SetPosition(JUMP_START_POS[i]);
		pObj->SetInfinity(true);
		pObj->SetShow(true);
		m_ParticleObjects[PARTICLE_SITUATION::JUMP][0].push_back(pObj);
		SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::JUMP][0].push_back(pInfo);
	}
	
	//jump particle
	for (int i = 0; i < MONSTER_NUM; ++i)
	{
		pObj = new CParticleObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pParticleTexture[PARTICLE_ADDRESS::COIN], pRandowmValueTexture, pRandowmValueOnSphereTexture, pShader, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), MAX_PARTICLES, PARTICLE_TYPE::COINBOMB);
		//Particle detail setting
		pInfo = new ParticleInfo();
		pObj->SettingDetail(PARTICLE_SITUATION::COINBYDEATH, i);

		m_ParticleObjects[PARTICLE_SITUATION::COINBYDEATH][0].push_back(pObj);
		SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::COINBYDEATH][0].push_back(pInfo);
	}
	
	//Fence particle
	for (int i = 0; i < 4; ++i)
	{
		pObj = new CParticleObject(pd3dDevice, pd3dCommandList, m_pd3dGraphicsRootSignature, m_pParticleTexture[PARTICLE_ADDRESS::ROUND], pRandowmValueTexture, pRandowmValueOnSphereTexture, pShader, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 0.0f), 0.0f, XMFLOAT3(0.0f, 0.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.0f, 1.0f), MAX_PARTICLES, PARTICLE_TYPE::FENCE);
		//Particle detail setting
		XMFLOAT3 FENCE_START_POS[4] = { XMFLOAT3(-129.95f, 3.71f, -80.5f), XMFLOAT3(-108.3f, 3.71f, -88.f), XMFLOAT3(-86.6f, 3.71f, -108.f), XMFLOAT3(-76.9f, 3.71f, -135.9f)};
		XMFLOAT3 FENCE_START_DIR[4] = { XMFLOAT3(0.02f, 0.f, 0.99f), XMFLOAT3(0.46f, 0.f, 0.88f), XMFLOAT3(0.84f, 0.f, 0.53f), XMFLOAT3(0.98f, 0.f, 0.15f)};
		pInfo = new ParticleInfo();
		pObj->SettingDetail(PARTICLE_SITUATION::FENCEEFFECT, i);

		pInfo->pos = FENCE_START_POS[i];
		pInfo->Dir = FENCE_START_DIR[i];
		pInfo->show = true;

		m_ParticleObjects[PARTICLE_SITUATION::FENCEEFFECT][0].push_back(pObj);
		SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::FENCEEFFECT][0].push_back(pInfo);
	}



	CreateShaderVariables(pd3dDevice, pd3dCommandList);

#ifdef Test
	cout << "Ingame Initialize Finish" << endl;
#endif // TEST
}

void CIngameScene::ReleaseUploadBuffers()
{
	if (m_pSkyBox) m_pSkyBox->ReleaseUploadBuffers();

	for (int i = 0; i < m_nShaders; i++) m_ppShaders[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nGameObjects; i++) if (m_ppGameObjects[i]) m_ppGameObjects[i]->ReleaseUploadBuffers();
	for (int i = 0; i < m_nHierarchicalGameObjects; i++) m_ppHierarchicalGameObjects[i]->ReleaseUploadBuffers();
	if(m_BillboardShader)m_BillboardShader->ReleaseUploadBuffers();
}

void CIngameScene::AnimateObjects(float fTimeElapsed)
{
	m_fElapsedTime = fTimeElapsed;
	CScene::AnimateObjects(fTimeElapsed);
	//reinterpret_cast<CBoundingBoxShader*>(m_ppShaders[0])->Update(m_pPlayer);
	XMFLOAT3 towerPos = XMFLOAT3();
	for (int i = 0; i < PATH_NUM; ++i)
	{
		auto tower = NetworkManager::GetInstance()->structureInfo[i];
		if (tower.active)
			towerPos = TOWER_POS[i];
	}
	//m_pLights[1].m_bEnable = towerPos.x != 0.f ? true : false;
	m_pLights[1].m_xmf3Position = towerPos;


	for (auto p : SceneManager::GetInstance()->m_ParticleInfo[PARTICLE_SITUATION::FENCEEFFECT][0])
	{
		p->show = NetworkManager::GetInstance()->magneticFenceActive;
	}

	reinterpret_cast<CGamePlayer*>(m_pPlayer)->UpdateRemaingTime();
	CTextureShader::GetInstance()->AnimateSpeedTexture(fTimeElapsed);
	CTextureShader::GetInstance()->AnimateGetDamagedTexture(fTimeElapsed);
}

void CIngameScene::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, bool bIsAnimated)
{
	CScene::Render(pd3dCommandList, pCamera, bIsAnimated);

	//Other Client Update && Render
	for (int i = 0; i < LOBBY_MAX_CLIENT; i++)
	{
		if (i == NetworkManager::GetInstance()->GetId())
			continue;

		if (m_ppOtherClient[i])
		{
			if (NetworkManager::GetInstance()->otherClientsInfo[i].show && bIsAnimated)
				m_ppOtherClient[i]->Animate(m_fElapsedTime);
			if (!m_ppOtherClient[i]->m_pSkinnedAnimationController)
				m_ppOtherClient[i]->UpdateTransform(NULL);

			if (NetworkManager::GetInstance()->otherClientsInfo[i].show) {
				m_ppOtherClient[i]->SetDissolveState(m_ppOtherClient[i]->m_nObjectDissolveState);
				if (NetworkManager::GetInstance()->otherClientsInfo[i].dissolve)
				{
					m_ppOtherClient[i]->SetAddDissolveState(m_fElapsedTime * 2);
					m_ppOtherClient[i]->m_fDissolveTime += m_fElapsedTime;
					if (m_ppOtherClient[i]->m_fDissolveTime > 3.5f)
					{
						NetworkManager::GetInstance()->otherClientsInfo[i].show = false;
						m_ppOtherClient[i]->m_fDissolveTime = 0.f;
						m_ppOtherClient[i]->m_nObjectDissolveState = 0;
					}
				}

				reinterpret_cast<COtherClientPlayer*>(m_ppOtherClient[i])->Update(i);
				if (i != 3) {
					reinterpret_cast<CPlayerObject*>(m_ppOtherClient[i])->Customize(NetworkManager::GetInstance()->m_ArrayInGameClientsCustom[i]);
					reinterpret_cast<CPlayerObject*>(m_ppOtherClient[i])->SetWeapon(static_cast<JOB>(NetworkManager::GetInstance()->readySceneInfo->playerJobs[i]));
				}
				m_ppOtherClient[i]->Render(pd3dCommandList, pCamera, m_ppOtherClient[i]->iMyShareNum);
			}
		}
	}

	//Minion Update && Render
	for (int i = 0; i < MAX_MINION; ++i) {
		if (m_minions[i]) {
			if (NetworkManager::GetInstance()->npcInfo[i].show)
			{
				if(bIsAnimated)m_minions[i]->Animate(m_fElapsedTime);
				if (!m_minions[i]->m_pSkinnedAnimationController) {
					m_minions[i]->UpdateTransform(NULL);
				}
				m_minions[i]->SetDissolveState(m_minions[i]->m_nObjectDissolveState);
				if (NetworkManager::GetInstance()->npcInfo[i].dissolve)
				{
					m_minions[i]->SetAddDissolveState(m_fElapsedTime * 2);
					m_minions[i]->m_fDissolveTime += m_fElapsedTime;
					if (m_minions[i]->m_fDissolveTime > 3.5f)
					{
						NetworkManager::GetInstance()->npcInfo[i].show = false;
						m_minions[i]->m_fDissolveTime = 0.f;
						m_minions[i]->m_nObjectDissolveState = 0;
						//m_minions[i]->SetDissolveState(0);
					}
				}
				reinterpret_cast<CNpc*>(m_minions[i])->Update(i);
				m_minions[i]->Render(pd3dCommandList, pCamera, m_minions[i]->iMyShareNum);
			}
			
		}
	}
	
	//Monster Update && Render
	for (int i = 0; i < MONSTER_NUM; ++i) {
		if (m_monsters[i]) {
			if (NetworkManager::GetInstance()->monsterInfo[i].show)
			{
				if(bIsAnimated)m_monsters[i]->Animate(m_fElapsedTime);
				if (!m_monsters[i]->m_pSkinnedAnimationController) {
					m_monsters[i]->UpdateTransform(NULL);
				}

				m_monsters[i]->SetDissolveState(m_monsters[i]->m_nObjectDissolveState);
				if (NetworkManager::GetInstance()->monsterInfo[i].dissolve)
				{
					m_monsters[i]->SetAddDissolveState(m_fElapsedTime * 2);
					m_monsters[i]->m_fDissolveTime += m_fElapsedTime;
					if (m_monsters[i]->m_fDissolveTime > 3.5f)
					{
						NetworkManager::GetInstance()->monsterInfo[i].show = false;
						m_monsters[i]->m_fDissolveTime = 0.f;
						m_monsters[i]->m_nObjectDissolveState = 0;
						//m_monsters[i]->SetDissolveState(0);
					}
				}
				reinterpret_cast<CMonster*>(m_monsters[i])->Update(i);
				m_monsters[i]->Render(pd3dCommandList, pCamera, m_monsters[i]->iMyShareNum);
			}	
		}
	}

	//Tower Object Render
	for (int i = 0; i < PATH_NUM; ++i) {
		if (m_towerAttacks[i]) {
			if (m_SkillTypeParticle[SKILL_TYPE::TOWERATTACK].size())
			{
				m_SkillTypeParticle[SKILL_TYPE::TOWERATTACK][i]->SetShow(reinterpret_cast<CTowerAttack*>(m_towerAttacks[i])->GetShow());
				if (m_SkillTypeParticle[SKILL_TYPE::TOWERATTACK][i]->GetShow())
				{
					m_SkillTypeParticle[SKILL_TYPE::TOWERATTACK][i]->SetPosition(m_towerAttacks[i]->GetPosition());
					m_SkillTypeParticle[SKILL_TYPE::TOWERATTACK][i]->SetForwardVector(m_towerAttacks[i]->GetLook());
				}
			}

			if (true == reinterpret_cast<CTowerAttack*>(m_towerAttacks[i])->GetShow()) {
				m_towerAttacks[i]->Render(pd3dCommandList, pCamera, m_towerAttacks[i]->iMyShareNum);
			}
		}
	}

	//Skill Object Update && Render
	for (auto& skills : m_skillObjects) {
		for (int i = 0; i < skills.second.size(); ++i) {
			//Particle Update
			if (m_SkillTypeParticle[skills.first].size())
			{
				m_SkillTypeParticle[skills.first][i]->SetShow(NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].show);
				if (m_SkillTypeParticle[skills.first][i]->GetShow())
				{
					m_SkillTypeParticle[skills.first][i]->SetPosition(NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].pos);
					m_SkillTypeParticle[skills.first][i]->SetForwardVector(NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].look);
				}
			}


			if (!NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].show)
				continue;
			skills.second[i]->Update(NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].pos, NetworkManager::GetInstance()->skillObjectInfos[skills.first][i].look);
			skills.second[i]->Render(pd3dCommandList, pCamera, skills.second[i]->iMyShareNum);
		}
	}
}

void CIngameScene::BillboardRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera, ID3D12DescriptorHeap* DescriptorHeap)
{
	m_BillboardShader->PostRender(pd3dCommandList, pCamera, DescriptorHeap);
}

