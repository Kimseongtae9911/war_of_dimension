#include "pch.h"
#include "CArcherAttack.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"
#include "GameUtil.h"

namespace wod_server {

	CArcherAttack::CArcherAttack()
	{
		const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherAttack);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = DirectX::XMFLOAT3(skillCsv->m_extent.m_x, skillCsv->m_extent.m_y, skillCsv->m_extent.m_z);
		m_maxVelXZ = skillCsv->m_speed;

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CArcherAttack::~CArcherAttack()
	{
	}

	bool CArcherAttack::Update(float _elapsedTime)
	{
		if (!m_active)
			return false;

		m_pos += m_look * m_maxVelXZ * _elapsedTime;
		UpdateBoundingBox();

		if (GameUtil::HeroSkillCollisionCheck(m_boundingBox, m_matchNum, m_power, m_critical, DAMAGE_TYPE::STRENGTH, m_clientID)) {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			m_active = false;
			return false;
		}
		else {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendUpdateSkillObjectPacket(m_id, m_type, m_pos);
			}
		}

		return true;
	}

	void CArcherAttack::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

}