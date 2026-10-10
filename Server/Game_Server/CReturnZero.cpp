#include "pch.h"
#include "CReturnZero.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CReturnZero::CReturnZero()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerReturn0);
		m_memoryLeak = m_skillCsv->m_debuffInfo[EDebuffType::MemoryLeak];
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { m_skillCsv->m_extent.m_x, m_skillCsv->m_extent.m_y, m_skillCsv->m_extent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CReturnZero::~CReturnZero()
	{
	}

	bool CReturnZero::Update(float _elapsedTime)
	{
		if (!m_active) {
			m_return = false;
			m_collideIDs.clear();
			return false;
		}

		m_pos += m_look * m_skillCsv->m_speed * _elapsedTime;
		UpdateBoundingBox();

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);
		if (GameUtil::SkillMapCollision(m_boundingBox)) {
			for (int id : clientIDs) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			return false;
		}

		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			if (m_collideIDs.contains(clientIDs[i]))
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (client->GetBoundingBox().Intersects(m_boundingBox)) {
				client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
				network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(client->GetID(), TimeUtil::CurTime(), EPlayerSkill::MemoryLeak, {}, static_cast<int>(m_power * m_memoryLeak.m_debuffValue), 0, {}, m_clientID));
				m_collideIDs.insert(clientIDs[i]);
			}
		}

		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, i);
			if (!npc->m_active)
				continue;
			if (m_collideIDs.contains(npc->GetID()))
				continue;
			if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
				npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC);
				m_collideIDs.insert(npc->GetID());
			}
		}

		for (int id : clientIDs) {
			if (id == -1)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendUpdateSkillObjectPacket(m_id, m_type, m_pos);
		}

		if (!m_return) {
			if (DistanceXZ(m_pos, m_startPos) > m_skillCsv->m_skillRadius) {
				m_return = true;
				m_look = vec3::Normalize(CObjectMgr::GetInstance()->GetClient(m_clientID)->GetPos() - m_pos);
				m_collideIDs.clear();
			}
		}
		else {
			if (DistanceXZ(m_pos, CObjectMgr::GetInstance()->GetClient(m_clientID)->GetPos()) < m_skillCsv->m_extraParam1) {
				for (int id : clientIDs) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
				}
				m_active = false;
				m_return = false;
				m_collideIDs.clear();
				return false;
			}
			m_look = vec3::Normalize(CObjectMgr::GetInstance()->GetClient(m_clientID)->GetPos() - m_pos);
		}

		return true;
	}


	void CReturnZero::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}
}
