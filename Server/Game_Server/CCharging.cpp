#include "pch.h"
#include "CCharging.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CNetworkMgr.h"
#include "CClient.h"
#include "CNpc.h"
#include "GameUtil.h"

namespace wod_server {

	CCharging::CCharging()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreCharging);
		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { GameUtil::GetBossPlayerInitBB().m_extent.m_x, GameUtil::GetBossPlayerInitBB().m_extent.m_y, GameUtil::GetBossPlayerInitBB().m_extent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CCharging::~CCharging()
	{
	}

	bool CCharging::Update(float _elapsedTime)
	{
		if (!m_active) {
			Reset();
			return false;
		}

		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(m_clientID);
		vec3 newPos = client->GetPos() + m_look * (m_skillCsv->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * _elapsedTime;
		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.m_y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);
			m_pos += m_look * (m_skillCsv->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * _elapsedTime;

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
			}
		}
		else {
			Reset();

			return false;
		}

		DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos(), client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, m_skillCsv->m_posOffset }, client->GetWorldMatrix());

		UpdateBoundingBox();

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (!m_collideObjects.contains(clientIDs[i])) {
				if (hero->GetBoundingBox().Intersects(box)) {
					hero->Damage(static_cast<int>(client->GetStatus()->GetStat().m_strength * m_skillCsv->m_strengthRatio), m_critical, DAMAGE_TYPE::STRENGTH, client->GetID());
					hero->GetStatus()->m_skillBuff = SKILL_BUFF::STUN;
					auto stunTime = m_skillCsv->m_debuffInfo[EDebuffType::Stun].m_debuffDuration;
					network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(hero->GetID(), TimeUtil::PassedTimeMSec(stunTime), EPlayerSkill::Stun, {}, 0, 0, {}));
					m_collideObjects.insert(clientIDs[i]);

					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
						if (-1 == id)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatusChangePakcet(hero->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::STUN));
					}
				}
			}
		}

		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
			if (!npc->m_active)
				continue;
			if (npc->GetBoundingBox().Intersects(box)) {
				npc->Damaged(client->GetID(), static_cast<int>(client->GetStatus()->GetStat().m_strength * m_skillCsv->m_strengthRatio), DAMAGE_TYPE::STRENGTH);
				npc->SetTargetClientID(client->GetID());
				npc->SetState(NPC_STATE::ST_CHASE);

				Reset();

				return false;
			}
		}

		return true;
	}


	void CCharging::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	void CCharging::Reset()
	{
		if (!m_collideObjects.empty()) {
			for (int id : m_collideObjects) {
				CObjectMgr::GetInstance()->GetClient(id)->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;
			}
			m_collideObjects.clear();
		}

		//There is a chance for packet send twice
		int matchID = CObjectMgr::GetInstance()->GetClient(m_clientID)->GetMatchId();
		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == id)
				continue;

			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendSkillFinishPacket(matchID);
		}

		CObjectMgr::GetInstance()->GetClient(m_clientID)->SetUsingSkill(false);
	}
}
