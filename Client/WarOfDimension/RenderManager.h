#pragma once

class CGameObject;
class CCamera;

class RenderManager
{
	SINGLETON(RenderManager);
public:
	void CalculateCameraDistance();
	void AddRenderVector(CGameObject* pObj);
	void OrderReder(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void SortbyOrder(float fTimeElapsed);

	void SetCameraPosition(XMFLOAT3 pos) { CameraPosition = pos; }

private:
	vector<CGameObject*> m_vecAlphaRenderObj;
	XMFLOAT3 CameraPosition = XMFLOAT3(0, 0, 0);
	float m_fSortTime = 0;
};

