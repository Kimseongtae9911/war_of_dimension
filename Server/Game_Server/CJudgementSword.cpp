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
		m_initBoundingBox.Extents = { m_skillCsv->extent.x,m_skillCsv->extent.y,m_skillCsv->extent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CJudgementSword::~CJudgementSword()
	{
	}

	bool CJudgementSword::Update(float elapsedTime)
	{
		if (!active)
			return false;

		if (m_targetID >= NPC_ID) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, m_targetID - NPC_ID);
			m_pos.x = npc->GetPos().x; m_pos.z = npc->GetPos().z;
			m_pos.y -= m_skillCsv->speed * elapsedTime;
			UpdateBoundingBox();

			if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
				network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_id, TimeUtil::PassedTimeMSec(1000), EPlayerSkill::SwordManJudgementSword, {}, m_matchNum, 2, {}, static_cast<int>(m_type)));
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC);
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
		}
		else {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(m_targetID);
			m_pos.x = client->GetPos().x; m_pos.z = client->GetPos().z;
			m_pos.y -= m_skillCsv->speed * elapsedTime;
			UpdateBoundingBox();

			if (client->GetBoundingBox().Intersects(m_boundingBox)) {
				network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_id, TimeUtil::PassedTimeMSec(1000), EPlayerSkill::SwordManJudgementSword, {}, m_matchNum, 2, {}, static_cast<int>(m_type)));

				client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
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
		}

		return true;
	}


	void CJudgementSword::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}
}