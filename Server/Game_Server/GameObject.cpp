#include "pch.h"
#include "GameObject.h"
#include "CClient.h"
#include "CNpc.h"

namespace wod_server {
	constexpr float TOWER_RANGE = 20.0f;
	constexpr float TOWER_ATTACK_SPEED = 15.0f;
	constexpr int TOWER_COOLTIME = 1;
	constexpr int TOWER_INIT_HP = 1000;
	constexpr int NEXUS_INIT_HP = 3000;

	void CMoveObject::UpdateBoundingBox()
	{
		m_worldMatrix._11 = m_right.x; m_worldMatrix._12 = m_right.y; m_worldMatrix._13 = m_right.z;
		m_worldMatrix._31 = m_look.x; m_worldMatrix._32 = m_look.y; m_worldMatrix._33 = m_look.z;
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	CTower::CTower(int matchNum, int id)
	{
		m_maxHp = m_curHp = TOWER_INIT_HP;
		//m_maxHp = m_curHp = 1;
		m_matchNum = matchNum;
		m_id = id;
		m_targetID = -1;
		active = false;
		m_broken = false;
		m_lastAttackTime = TimeUtil::CurTime();
	}

	CTower::~CTower()
	{
	}

	void CTower::Update(int matchNum)
	{
		if (!active)
			return;
		if (std::chrono::duration_cast<std::chrono::seconds>(TimeUtil::CurTime() - m_lastAttackTime) <= std::chrono::seconds(TOWER_COOLTIME))
			return;

		if (-1 != m_targetID) {
			std::shared_ptr<CMoveObject> target;
			bool targetRemoved = false;
			if (m_targetID < NPC_ID) {
				//Target is Boss
				target = CObjectMgr::GetInstance()->GetClient(m_targetID);
				targetRemoved = reinterpret_cast<CClient*>(target.get())->IsDead();
			}
			else {
				//Target is Minion
				target = CObjectMgr::GetInstance()->GetNpc(matchNum, m_targetID - NPC_ID);
				targetRemoved = !reinterpret_cast<CNpc*>(target.get())->active;
			}
			if (targetRemoved) {
				//Target removed
				m_targetID = -1;
			}
			else if (DistanceXZ(m_pos, target->GetPos()) > TOWER_RANGE) {
				//Target out of range
				m_targetID = -1;
			}
			else {
				//Target still in range
				CGameMgr::GetInstance()->TowerAttack(matchNum, m_targetID, m_pos);
				m_lastAttackTime = TimeUtil::CurTime();
				return;
			}
		}

		int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)[3];
		if (bossID != -1) {
			std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);
			if (DistanceXZ(m_pos, boss->GetPos()) <= TOWER_RANGE) {
				m_targetID = boss->GetID();
				CGameMgr::GetInstance()->TowerAttack(matchNum, m_targetID, m_pos);
				m_lastAttackTime = TimeUtil::CurTime();
				return;
			}
		}

		for (int i = 0; i < MAX_MINION; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
			if (!npc->active)
				continue;

			//Attack target
			if (DistanceXZ(m_pos, npc->GetPos()) <= TOWER_RANGE) {
				m_targetID = npc->GetID();
				CGameMgr::GetInstance()->TowerAttack(matchNum, m_targetID, m_pos);
				m_lastAttackTime = TimeUtil::CurTime();
				break;
			}
		}
	}

	void CTower::Damage(int damage)
	{
		m_hpLock.lock();
		m_curHp -= damage;
		m_hpLock.unlock();

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendStructureStatChangePacket(m_id, m_maxHp, m_curHp);
		}

		if (m_curHp <= 0) {
			m_broken = true;
			active = false;

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendStructureStatusChangePacket(m_id, true);
			}

			CGameMgr::GetInstance()->SetNexusAttackPossible(m_matchNum, true);
		}

	}

	void CTower::Reset()
	{
		m_maxHp = m_curHp = TOWER_INIT_HP;
		m_targetID = -1;
		active = false;
		m_broken = false;
		m_lastAttackTime = TimeUtil::CurTime();
		if (m_id == 3)
			active = true;
	}


	CTowerAttack::CTowerAttack()
	{
		active = false;
		m_targetID = -1;
		m_vel = {TOWER_ATTACK_SPEED, TOWER_ATTACK_SPEED , TOWER_ATTACK_SPEED };

		m_initBoundingBox.Center = { 0.0f, 0.0f, 0.0f };
		m_initBoundingBox.Extents = { 0.5f, 0.5f, 0.5f };
		
		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
	}

	CTowerAttack::~CTowerAttack()
	{
	}
	
	bool CTowerAttack::Update(float elapsedTime)
	{
		if (!active || -1 == m_targetID)
			return false;

		UpdateBoundingBox();

		//Move Toward Target
		std::shared_ptr<CMoveObject> target;
		bool remove = false;
		auto clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);

		if (m_targetID < NPC_ID) {
			target = CObjectMgr::GetInstance()->GetClient(m_targetID);
			if (m_boundingBox.Intersects(target->GetBoundingBox())) {
				remove = true;
				reinterpret_cast<CClient*>(target.get())->Damage((TOWER_INIT_POWER + static_cast<int>(CGameMgr::GetInstance()->GetGameTime(m_matchNum) / 60) * TOWER_POWER_INCREASE), 0, DAMAGE_TYPE::STRENGTH, 0);
				if (reinterpret_cast<CClient*>(target.get())->GetStatus()->healthMana.GetCurHp() <= 0) {
					m_targetID = -1;
				}
			}
		}
		else {
			target = CObjectMgr::GetInstance()->GetNpc(m_matchNum, m_targetID - NPC_ID);

			if (m_boundingBox.Intersects(target->GetBoundingBox())) {
				remove = true;
				if (reinterpret_cast<CNpc*>(target.get())->Damaged(0, (TOWER_INIT_POWER + static_cast<int>(CGameMgr::GetInstance()->GetGameTime(m_matchNum) / 60) * TOWER_POWER_INCREASE), DAMAGE_TYPE::STRENGTH, false)) {
					m_targetID = -1;
				}
			}
		}

		m_look = vec3::Normalize(target->GetPos() - m_pos);
		m_pos += (m_look * m_vel * elapsedTime);

		if (remove) {
			active = false;
			for (int i = 0; i < MAX_PLAYER; ++i) {
				if (-1 == clientIDs[i])
					continue;
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
				client->GetPacketSender()->SendTowerAttackRemovePacket(m_id);
			}
		}

		//Send Packet of TowerAttack Object
		for (int i = 0; i < MAX_PLAYER; ++i) {
			if (-1 == clientIDs[i])
				continue;
			CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPacketSender()->SendTowerAttackPacket(m_id, m_pos, m_look);
		}

		return true;
	}

	void CTowerAttack::UpdateBoundingBox()
	{
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	CNexus::CNexus(int matchNum)
	{
		m_maxHp = m_curHp = NEXUS_INIT_HP;
		m_pos = { -123.2651f, 3.700449f, -127.6417f };
		m_matchNum = matchNum;
	}

	void CNexus::Damage(int damage)
	{
		if (!CGameMgr::GetInstance()->GetNexusAttackPossible(m_matchNum))
			return;

		m_hpLock.lock();
		m_curHp -= damage;
		m_hpLock.unlock();

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendStructureStatChangePacket(4, m_maxHp, m_curHp);
		}

		if (m_curHp <= 0) {
			CGameMgr::GetInstance()->GameOver(m_matchNum, false);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendGameOverPacket(true);
			}
		}
	}

	void CNexus::Reset()
	{
		m_maxHp = m_curHp = NEXUS_INIT_HP;
	}
}