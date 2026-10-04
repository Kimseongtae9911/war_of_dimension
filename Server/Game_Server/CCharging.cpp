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
		m_initBoundingBox.Extents = { GameUtil::GetBossPlayerInitBB().extent.x, GameUtil::GetBossPlayerInitBB().extent.y, GameUtil::GetBossPlayerInitBB().extent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CCharging::~CCharging()
	{
	}

	bool CCharging::Update(float elapsedTime)
	{
		if (!active) {
			Reset();
			return false;
		}

		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(m_clientID);
		vec3 newPos = client->GetPos() + m_look * (m_skillCsv->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * elapsedTime;
		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);
			m_pos += m_look * (m_skillCsv->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * elapsedTime;

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

		DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos(), client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, m_skillCsv->posOffset }, client->GetWorldMatrix());

		UpdateBoundingBox();

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (!m_collideObjects.contains(clientIDs[i])) {
				if (hero->GetBoundingBox().Intersects(box)) {
					hero->Damage(static_cast<int>(client->GetStatus()->GetStat().strength * m_skillCsv->strengthRatio), m_critical, DAMAGE_TYPE::STRENGTH, client->GetID());
					hero->GetStatus()->skillBuff = SKILL_BUFF::STUN;
					auto stunTime = m_skillCsv->debuffInfo[EDebuffType::Stun].debuffDuration;
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
			if (!npc->active)
				continue;
			if (npc->GetBoundingBox().Intersects(box)) {
				npc->Damaged(client->GetID(), static_cast<int>(client->GetStatus()->GetStat().strength * m_skillCsv->strengthRatio), DAMAGE_TYPE::STRENGTH);
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
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	void CCharging::Reset()
	{
		if (!m_collideObjects.empty()) {
			for (int id : m_collideObjects) {
				CObjectMgr::GetInstance()->GetClient(id)->GetStatus()->skillBuff = SKILL_BUFF::NONE;
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
