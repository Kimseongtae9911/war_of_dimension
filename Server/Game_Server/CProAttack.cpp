#include "pch.h"
#include "CProAttack.h"
#include "CClient.h"

namespace wod_server {

	CProAttack::CProAttack()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerAttack);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { m_skillCsv->extent.x, m_skillCsv->extent.y, m_skillCsv->extent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CProAttack::~CProAttack()
	{
	}

	bool CProAttack::Update(float elapsedTime)
	{
		if (!active)
			return false;

		m_pos += m_look * m_skillCsv->speed * elapsedTime;
		UpdateBoundingBox();

		//Tower Collide Check
		for (int i = 0; i < PATH_NUM; ++i) {
			if (CGameMgr::GetInstance()->GetTower(m_matchNum, i)->GetBroken() || !CGameMgr::GetInstance()->GetTower(m_matchNum, i)->active)
				continue;
			if (GameUtil::GetTowerBB(i).Intersects(m_boundingBox)) {
				CGameMgr::GetInstance()->GetTower(m_matchNum, i)->Damage(m_power);
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
				}
				active = false;
				return false;
			}
		}

		//Nexus Collide Check
		if (GameUtil::GetNexusBB().Intersects(m_boundingBox)) {
			CGameMgr::GetInstance()->GetNexus(m_matchNum)->Damage(m_power);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			active = false;
			return false;
		}


		if (GameUtil::BossSkillCollisionCheck(m_boundingBox, m_matchNum, m_power, m_critical, DAMAGE_TYPE::STRENGTH, m_clientID)) {
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

	void CProAttack::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

}