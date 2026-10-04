#include "pch.h"
#include "CMagicEye.h"
#include "CNetworkMgr.h"
#include "CClient.h"

namespace wod_server {
	constexpr float MAGIC_EYE_DISTANCE = 15.f;

	CMagicEye::CMagicEye()
	{
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { 0.125f, 0.125f, 0.125f };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CMagicEye::~CMagicEye()
	{
	}

	bool CMagicEye::Update(float elapsedTime)
	{
	
		return false;
	}
}
