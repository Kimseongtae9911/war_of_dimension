#include "pch.h"
#include "CJudgementSword.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CNetworkMgr.h"
#include "CClient.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CJudgementSword::CJudgementSword()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManJudgementSword);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { m_skillCsv->m_extent.m_x,m_skillCsv->m_extent.m_y,m_skillCsv->m_extent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CJudgementSword::~CJudgementSword()
	{
	}

	bool CJudgementSword::Update(float _elapsedTime)
	{
		if (!m_active)
			return false;

		if (m_targetID >= NPC_ID) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, m_targetID - NPC_ID);
			m_pos.m_x = npc->GetPos().m_x; m_pos.m_z = npc->GetPos().m_z;
			m_pos.m_y -= m_skillCsv->m_speed * _elapsedTime;
			UpdateBoundingBox();

			if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
				network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_id, TimeUtil::PassedTimeMSec(1000), EPlayerSkill::SwordManJudgementSword, {}, m_matchNum, 2, {}, static_cast<int>(m_type)));
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC);
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
		}
		else {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(m_targetID);
			m_pos.m_x = client->GetPos().m_x; m_pos.m_z = client->GetPos().m_z;
			m_pos.m_y -= m_skillCsv->m_speed * _elapsedTime;
			UpdateBoundingBox();

			if (client->GetBoundingBox().Intersects(m_boundingBox)) {
				network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_id, TimeUtil::PassedTimeMSec(1000), EPlayerSkill::SwordManJudgementSword, {}, m_matchNum, 2, {}, static_cast<int>(m_type)));

				client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
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
		}

		return true;
	}


	void CJudgementSword::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}
}