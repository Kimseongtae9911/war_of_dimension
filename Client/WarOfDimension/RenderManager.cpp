#include "stdafx.h"
#include "RenderManager.h"
#include "Object.h"
#include <algorithm>

std::unique_ptr<RenderManager> RenderManager::m_instance;

float CalculateDistance(const XMFLOAT3& point1, const XMFLOAT3& point2)
{
    XMVECTOR vec1 = XMLoadFloat3(&point1);
    XMVECTOR vec2 = XMLoadFloat3(&point2);
    XMVECTOR distanceVec = XMVectorSubtract(vec1, vec2);
    distanceVec = XMVector3Length(distanceVec);

    float distance;
    XMStoreFloat(&distance, distanceVec);

    return distance;
}

void RenderManager::CalculateCameraDistance()
{
    for (auto p : m_vecAlphaRenderObj)
    {
        p->m_fDistanceCamera = CalculateDistance(CameraPosition, p->GetPosition());
    }
}

void RenderManager::AddRenderVector(CGameObject* pObj)
{
    m_vecAlphaRenderObj.push_back(pObj);

    if (pObj->m_pSibling) AddRenderVector(pObj->m_pSibling);
    if (pObj->m_pChild) AddRenderVector(pObj->m_pChild);
}

void RenderManager::OrderReder(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
    for (auto p : m_vecAlphaRenderObj)
    {
        p->PureRender(pd3dCommandList, pCamera);
    }
}

void RenderManager::SortbyOrder(float fTimeElapsed)
{
    m_fSortTime += fTimeElapsed;
    if (m_fSortTime > 1.f)
    {
        sort(m_vecAlphaRenderObj.begin(), m_vecAlphaRenderObj.end(), [](CGameObject* a, CGameObject* b) {return a->m_fDistanceCamera > b->m_fDistanceCamera; });
        m_fSortTime = 0;
    }   
}

