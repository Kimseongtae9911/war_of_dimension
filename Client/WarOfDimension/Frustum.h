#pragma once

enum PLANE_TYPE
{
	PLANE_TOP,
	PLANE_BOTTOM,
	PLANE_LEFT,
	PLANE_RIGHT,
	PLANE_NEAR,
	PLANE_FAR,
	PLANE_END
};
class Frustum
{
public:
	static Frustum* Create();
	HRESULT Initialize();
	static Frustum* GetInstance() { return s_instance; }

public:
	void Update();
	bool CheckAABB(const XMFLOAT3& center, const XMFLOAT3& extents);
	bool CheckPoint(XMFLOAT3 center);
	bool CheckSphere(const XMFLOAT3& center, float radius);
	void Print();

	
public:
	XMFLOAT4X4						m_xmfCamera4x4View;
	XMFLOAT4X4						m_xmf4x4CameraProjection;
	bool							m_bToggle = true;
	int								m_iCountRender = 0;
private:
	array<XMFLOAT4, PLANE_END> m_FrustumPlanes;
	static Frustum* s_instance;
};

