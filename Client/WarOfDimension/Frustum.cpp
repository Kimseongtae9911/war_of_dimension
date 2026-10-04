#include "stdafx.h"
#include "Frustum.h"


Frustum* Frustum::s_instance = nullptr;

Frustum* Frustum::Create()
{
    Frustum* pInstance = new Frustum();
    s_instance = pInstance;
    if (FAILED(pInstance->Initialize()))
    {
        SafeDelete(pInstance);
        return nullptr;
    }

    return pInstance;
}

HRESULT Frustum::Initialize()
{
    return NOERROR;
}

void Frustum::Update()
{
     
    XMFLOAT4X4 combinedMatrix;
    XMStoreFloat4x4(&combinedMatrix, XMMatrixMultiply(XMLoadFloat4x4(&m_xmfCamera4x4View), XMLoadFloat4x4(&m_xmf4x4CameraProjection)));

    m_FrustumPlanes[PLANE_LEFT].x = combinedMatrix._14 + combinedMatrix._11;
    m_FrustumPlanes[PLANE_LEFT].y = combinedMatrix._24 + combinedMatrix._21;
    m_FrustumPlanes[PLANE_LEFT].z = combinedMatrix._34 + combinedMatrix._31;
    m_FrustumPlanes[PLANE_LEFT].w = combinedMatrix._44 + combinedMatrix._41;

    m_FrustumPlanes[PLANE_RIGHT].x = combinedMatrix._14 - combinedMatrix._11;
    m_FrustumPlanes[PLANE_RIGHT].y = combinedMatrix._24 - combinedMatrix._21;
    m_FrustumPlanes[PLANE_RIGHT].z = combinedMatrix._34 - combinedMatrix._31;
    m_FrustumPlanes[PLANE_RIGHT].w = combinedMatrix._44 - combinedMatrix._41;

    // Top Frustum Plane
    // Subtract second column of matrix from the fourth column
    m_FrustumPlanes[PLANE_TOP].x = combinedMatrix._14 - combinedMatrix._12;
    m_FrustumPlanes[PLANE_TOP].y = combinedMatrix._24 - combinedMatrix._22;
    m_FrustumPlanes[PLANE_TOP].z = combinedMatrix._34 - combinedMatrix._32;
    m_FrustumPlanes[PLANE_TOP].w = combinedMatrix._44 - combinedMatrix._42;

    // Bottom Frustum Plane
    // Add second column of the matrix to the fourth column
    m_FrustumPlanes[PLANE_BOTTOM].x = combinedMatrix._14 + combinedMatrix._12;
    m_FrustumPlanes[PLANE_BOTTOM].y = combinedMatrix._24 + combinedMatrix._22;
    m_FrustumPlanes[PLANE_BOTTOM].z = combinedMatrix._34 + combinedMatrix._32;
    m_FrustumPlanes[PLANE_BOTTOM].w = combinedMatrix._44 + combinedMatrix._42;

    // Near Frustum Plane
    // We could add the third column to the fourth column to get the near plane,
    // but we don't have to do this because the third column IS the near plane
    m_FrustumPlanes[PLANE_NEAR].x = combinedMatrix._13;
    m_FrustumPlanes[PLANE_NEAR].y = combinedMatrix._23;
    m_FrustumPlanes[PLANE_NEAR].z = combinedMatrix._33;
    m_FrustumPlanes[PLANE_NEAR].w = combinedMatrix._43;

    // Far Frustum Plane
    // Subtract third column of matrix from the fourth column
    m_FrustumPlanes[PLANE_FAR].x = combinedMatrix._14 - combinedMatrix._13;
    m_FrustumPlanes[PLANE_FAR].y = combinedMatrix._24 - combinedMatrix._23;
    m_FrustumPlanes[PLANE_FAR].z = combinedMatrix._34 - combinedMatrix._33;
    m_FrustumPlanes[PLANE_FAR].w = combinedMatrix._44 - combinedMatrix._43;

    for (int i = 0; i < PLANE_END; ++i)
    {
        
        float length = sqrt((m_FrustumPlanes[i].x * m_FrustumPlanes[i].x) + (m_FrustumPlanes[i].y * m_FrustumPlanes[i].y) + (m_FrustumPlanes[i].z * m_FrustumPlanes[i].z));
        m_FrustumPlanes[i].x /= length;
        m_FrustumPlanes[i].y /= length;
        m_FrustumPlanes[i].z /= length;
        m_FrustumPlanes[i].w /= length;
    }
}

bool Frustum::CheckAABB(const XMFLOAT3& center, const XMFLOAT3& extents) 
{
    for (int i = 0; i < PLANE_END; ++i)
    {
        // AABB의 꼭지점과 평면 사이의 거리 계산
        XMFLOAT3 positiveVertex;
        positiveVertex.x = (m_FrustumPlanes[i].x >= 0.0f) ? center.x - extents.x : center.x + extents.x;
        positiveVertex.y = (m_FrustumPlanes[i].y >= 0.0f) ? center.y - extents.y : center.y + extents.y;
        positiveVertex.z = (m_FrustumPlanes[i].z >= 0.0f) ? center.z - extents.z : center.z + extents.z;

        float distance = m_FrustumPlanes[i].x * positiveVertex.x +
            m_FrustumPlanes[i].y * positiveVertex.y +
            m_FrustumPlanes[i].z * positiveVertex.z +
            m_FrustumPlanes[i].w;

        // AABB가 평면의 반대쪽에 있는 경우 컬링
        if (distance < 0.0f)
            return false;
    }
    if (!m_bToggle)
    {
        cout << "x : "<<extents.x << "y : " << extents.y << "z : " <<extents.z << endl;
    }
    // 모든 평면을 통과한 경우 컬링되지 않음
    return true;
}


bool Frustum::CheckSphere(const XMFLOAT3& center, float radius)
{
    for (int i = 0; i < PLANE_END; ++i)
    {
        float distance = (m_FrustumPlanes[i].x * center.x) + (m_FrustumPlanes[i].y * center.y) +
            (m_FrustumPlanes[i].z * center.z) + m_FrustumPlanes[i].w + 10.f;

        if (distance < -radius)
            return false; // 구가 평면의 반대쪽에 있는 경우

        //if (fabs(distance) < radius)
        //{
        //    ++m_iCountRender;
        //    return true; // 구가 평면에 걸치는 경우
        //}
            
    }
    ++m_iCountRender;
    return true; // 구가 모든 평면에 완전히 포함되는 경우
}


bool Frustum::CheckPoint(XMFLOAT3 center)
{
    for (int i = 0; i < PLANE_END; ++i)
    {
        float distance = m_FrustumPlanes[i].x * center.x +
            m_FrustumPlanes[i].y * center.y +
            m_FrustumPlanes[i].z * center.z +
            m_FrustumPlanes[i].w + 10.f;
        // 만약 점이 평면의 반대쪽에 있다면 컬링
        if (distance < 0.0f)
            return false;
    }

    // 모든 평면을 통과한 경우 컬링되지 않음
    return true;
}

void Frustum::Print()
{

    cout << m_iCountRender << endl;

}
