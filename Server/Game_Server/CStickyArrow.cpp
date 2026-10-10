#include "pch.h"
#include "CStickyArrow.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CNetworkMgr.h"
#include "CClient.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CStickyArrow::CStickyArrow()
	{
		const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherAttack);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = DirectX::XMFLOAT3(skillCsv->m_extent.m_x, skillCsv->m_extent.m_y, skillCsv->m_extent.m_z);
		m_maxVelXZ = skillCsv->m_speed;

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CStickyArrow::~CStickyArrow()
	{
	}

	bool CStickyArrow::Update(float _elapsedTime)
	{
		if (!m_active) {
			return false;
		}

		m_pos += m_look * m_maxVelXZ * _elapsedTime;
		UpdateBoundingBox();

		if (GameUtil::SkillMapCollision(m_boundingBox)) {
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
			}
			m_active = false;
			return false;
		}
		else {
			int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)[3];
			if (bossID != -1) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(bossID);
				if (client->GetBoundingBox().Intersects(m_boundingBox)) {
					client->Damage(m_power, m_critical, DAMAGE_TYPE::STRENGTH, client->GetID());
					CStat stat = client->GetStatus()->GetStat();
					stat.m_speed -= m_slowDebuff.m_debuffValue;
					client->GetStatus()->SetStat(stat);

					CStat changeStat(0);
					changeStat.m_speed = m_slowDebuff.m_debuffValue * (-1.f);
					network::GetInstance()->RegisterTimerEvent({ bossID, TimeUtil::PassedTimeMSec(m_slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });

					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
					}
					m_active = false;
					return false;
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, i);
				if (!npc->m_active)
					continue;
				if (npc->GetBoundingBox().Intersects(m_boundingBox)) {
					npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::STRENGTH);

					float speed = npc->GetSpeed();
					speed -= m_slowDebuff.m_debuffValue;
					npc->SetSpeed(speed);

					CStat changeStat(0);
					changeStat.m_speed = m_slowDebuff.m_debuffValue * (-1.f);
					network::GetInstance()->RegisterTimerEvent({ npc->GetID(), TimeUtil::PassedTimeMSec(m_slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, m_matchNum, changeStat});

					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(m_id, m_type);
					}
					m_active = false;
					return false;
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

	void CStickyArrow::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

}