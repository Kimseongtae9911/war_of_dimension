#include "pch.h"
#include "CEnergyBall.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"
#include "GameUtil.h"

namespace wod_server {

	CEnergyBall::CEnergyBall()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardEnergyBall);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { m_skillCsv->extent.x, m_skillCsv->extent.y, m_skillCsv->extent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CEnergyBall::~CEnergyBall()
	{
	}

	bool CEnergyBall::Update(float elapsedTime)
	{
		if (!active)
			return false;

		m_pos += m_look * m_skillCsv->speed * elapsedTime;
		UpdateBoundingBox();

		if (GameUtil::HeroSkillCollisionCheck(m_boundingBox, m_matchNum, m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID)) {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			active = false;
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


	void CEnergyBall::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}
}
