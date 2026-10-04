#include "pch.h"
#include "CPhoenixArrow.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CPhoenixArrow::CPhoenixArrow()
	{
		const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherPhoenixArrow);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = DirectX::XMFLOAT3(skillCsv->extent.x, skillCsv->extent.y, skillCsv->extent.z);

		m_burnDebuff = skillCsv->debuffInfo.find(EDebuffType::Burn)->second;
		m_maxVelXZ = skillCsv->speed;

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CPhoenixArrow::~CPhoenixArrow()
	{
	}

	bool CPhoenixArrow::Update(float elapsedTime)
	{
		if (!active) {
			return false;
		}

		m_pos += m_look * m_maxVelXZ * elapsedTime;
		UpdateBoundingBox();

		if (GameUtil::SkillMapCollision(m_boundingBox)) {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			active = false;
			return false;
		}
		else {
			int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)[3];
			if (bossID != -1) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(bossID);
				if (client->GetBoundingBox().Intersects(m_boundingBox)) {
					client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
					network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(bossID, TimeUtil::PassedTimeMSec(m_burnDebuff.debuffDuration), EPlayerSkill::Burn, vec3(), static_cast<int>(m_power * m_burnDebuff.debuffValue), 0, {}, m_clientID));
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, i);
				if (!npc->active)
					continue;
				if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
					npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC);

					network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(npc->GetID(), TimeUtil::PassedTimeMSec(m_burnDebuff.debuffDuration), EPlayerSkill::Burn, vec3(static_cast<float>(m_matchNum), 0, 0), static_cast<int>(m_power * m_burnDebuff.debuffValue), 0, {}, m_clientID));
				}
			}


			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendUpdateSkillObjectPacket(m_id, m_type, m_pos);
			}
		}

		return true;
	}

	void CPhoenixArrow::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

}