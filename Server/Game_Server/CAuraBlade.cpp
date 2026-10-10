#include "pch.h"
#include "CAuraBlade.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CAuraBlade::CAuraBlade()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManAuraBlade);

		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { m_skillCsv->m_extent.m_x, m_skillCsv->m_extent.m_y, m_skillCsv->m_extent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CAuraBlade::~CAuraBlade()
	{
	}

	bool CAuraBlade::Update(float _elapsedTime)
	{
		if (!m_active) {
			if (!m_collideIDs.empty()) {
				m_collideIDs.clear();
			}
			return false;
		}

		m_pos += m_look * m_skillCsv->m_speed * _elapsedTime;
		UpdateBoundingBox();

		std::array<int, MAX_PLAYER> playerIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);

		if (GameUtil::SkillMapCollision(m_boundingBox)) {
			for (int id : playerIDs) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			m_active = false;
			if (!m_collideIDs.empty()) {
				m_collideIDs.clear();
			}
			return false;
		}

		int bossID = playerIDs[3];
		if (bossID != -1) {
			if (!m_collideIDs.contains(bossID)) {
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);
				if (boss->GetBoundingBox().Intersects(m_boundingBox)) {
					boss->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
					m_collideIDs.insert(bossID);
					m_power -= static_cast<int>(m_power * m_skillCsv->m_damageReduction);
				}
			}
		}

		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, i);
			if (!npc->m_active)
				continue;
			if (m_collideIDs.contains(npc->GetID()))
				continue;
			if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
				npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC);
				m_collideIDs.insert(npc->GetID());
				m_power -= static_cast<int>(m_power * m_skillCsv->m_damageReduction);
			}
		}

		for (int id : playerIDs) {
			if (id == -1)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendUpdateSkillObjectPacket(m_id, m_type, m_pos);
		}

		return true;
	}

	void CAuraBlade::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}
}
