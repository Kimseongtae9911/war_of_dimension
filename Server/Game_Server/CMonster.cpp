#include "pch.h"
#include "GameObject.h"
#include "CNpc.h"
#include "CMonster.h"

namespace wod_server {
	void CMonster::Initialize(uint8_t _posIndex)
	{
		int8_t monsterId = m_id - NPC_ID;

		auto npcCsv = NpcCsvMgr::GetInstance()->GetNpcCsv(m_npcType);
		if (nullptr == npcCsv)
			return;

        if (_posIndex >= npcCsv->respawnPos.size() || _posIndex >= npcCsv->respawnLook.size())
            throw std::out_of_range("NPC spawn index");

		uint32_t maxHp = static_cast<uint32_t>(npcCsv->BaseHp) + static_cast<uint32_t>((CGameMgr::GetInstance()->GetGameTime(m_matchNum) / TimeUtil::Min) * npcCsv->HpIncrease);
		const auto monster = static_cast<CMonster*>(CObjectMgr::GetInstance()->GetNpc(m_matchNum, monsterId).get());

		monster->InitializeHp(maxHp);
		monster->SetInitPos(npcCsv->respawnPos[_posIndex]);
		monster->SetInitLook(npcCsv->respawnLook[_posIndex]);
		monster->SetPower(npcCsv->BaseAttack);
		monster->SetRespawnNum(_posIndex);

		for (int clId : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (clId == -1)
				continue;

			CObjectMgr::GetInstance()->GetClient(clId)->GetPacketSender()->SendAddNpcPacket(monsterId, monster->GetPos(), monster->GetLook(), monster->GetRight(), NPC_TYPE::UNIQUE_DRAGON);
			CObjectMgr::GetInstance()->GetClient(clId)->GetPacketSender()->SendNpcStatChangePacket(monsterId, monster->GetMaxHp(), monster->GetCurHp());
		}
		monster->active = true;		
	}

	bool CMonster::Update(float elapsedTime)
	{
		if (NPC_STATE::ST_CHASE == m_state) {
			LookTarget();
			Chase(elapsedTime);

			if (DistanceXZ(m_pos, m_initPos) > m_chaseDistance) {
				m_state = NPC_STATE::ST_RETURN;
				Heal();
			}
			else if (DistanceXZ(m_pos, CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos()) < m_attackDistance) {
				m_state = NPC_STATE::ST_ATTACK;
			}
		}
		else if (NPC_STATE::ST_RETURN == m_state) {
			ReturnPos(elapsedTime);
			if (DistanceXZ(m_pos, m_initPos) < 0.1f) {
				m_pos = m_initPos;
				SetLook(m_initLook);
				m_state = NPC_STATE::ST_IDLE;
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right, NPC_TYPE::MINION, true);
				}
			}
		}
		else if (NPC_STATE::ST_ATTACK == m_state) {
			Attack(elapsedTime);
			if (-1 == m_targetClientID)
				return true;
			if (DistanceXZ(m_pos, CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos()) > m_attackDistance) {
				m_state = NPC_STATE::ST_CHASE;
			}
		}
		else {
			UpdateBoundingBox();
		}

		return true;
	}

	void CMonster::Chase(float elapsedTime)
	{
		if (m_attack) {
			Rotate(elapsedTime);
			return;
		}
		else {
			if (-1 == m_targetClientID) {
				m_state = NPC_STATE::ST_RETURN;
				Heal();
				return;
			}
			vec3 clientPos = CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos();
			m_targetPos.x = clientPos.x;
			m_targetPos.z = clientPos.z;
			LookTarget();
		}

		if (m_rotate)
			Rotate(elapsedTime);

		Move(elapsedTime);
	}

	bool CMonster::Damaged(int clientID, int power, DAMAGE_TYPE type, bool updateTarget)
	{
		if (CNpc::Damaged(clientID, power, type, updateTarget)) {
			//Respawn
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientID);
			network::GetInstance()->RegisterTimerEvent({ m_id, TimeUtil::PassedTimeMSec(m_respawnTime), EVENT_TYPE::EV_NPC_ACTIVE, m_matchNum });
			m_state = NPC_STATE::ST_IDLE;

			int increasedGold;
			if (client->GetMatchId() == 3) {
				increasedGold = client->GetGold() + static_cast<int>(m_gold * 1.5f);
				client->SetGold(increasedGold);
			}
			else {
				increasedGold = client->GetGold() + m_gold;
				client->SetGold(increasedGold);
			}
			client->GetPacketSender()->SendGoldPacket(client->GetMatchId(), increasedGold);

			if (m_npcCsv->BuffInfos.size() != 0) {
				RegisterKillBuff(clientID);
			}

			return true;
		}
		else if (updateTarget) {
			SetTargetClientID(clientID);
			m_state = NPC_STATE::ST_CHASE;
		}

		return false;
	}

	void CMonster::ReturnPos(float elapsedTime)
	{
		if (m_attack) {
			m_attack = false;
		}

		else {
			vec3 clientPos = CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos();
			m_targetPos.x = m_initPos.x;
			m_targetPos.z = m_initPos.z;
			LookTarget();
		}

		if (m_rotate)
			Rotate(elapsedTime);

		Move(elapsedTime);
	}

	void CMonster::Respawn(int _currTime)
	{
		InitializeHp(static_cast<int>(m_npcCsv->BaseHp) + _currTime * m_npcCsv->HpIncrease);
		SetPos(m_npcCsv->respawnPos[m_respawnNum]);
		SetLook(vec3::Normalize(m_npcCsv->respawnLook[m_respawnNum]));
		SetPower(m_npcCsv->BaseAttack + _currTime * m_npcCsv->AttackIncrease);

		SetTargetClientID(-1);
		SetState(NPC_STATE::ST_IDLE);
		active = true;


		for (int clID : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (clID == -1)
				continue;
			CObjectMgr::GetInstance()->GetClient(clID)->GetPacketSender()->SendAddNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
			CObjectMgr::GetInstance()->GetClient(clID)->GetPacketSender()->SendNpcStatChangePacket(m_id - NPC_ID, m_maxHp, m_curHp);
		}
	}

	void CMonster::SetMonsterInfo()
	{

		auto npcCsv = NpcCsvMgr::GetInstance()->GetNpcCsv(m_npcType);
		if (npcCsv == nullptr)
		{
			LogPrinter::PrintMsg("UniqueRed Csv is not valid");
			return;
		}
		m_npcCsv = npcCsv;

		m_maxHp = m_curHp = static_cast<int>(npcCsv->BaseHp);
		m_maxVelXZ = npcCsv->MaxSpeed;
		m_state = NPC_STATE::ST_IDLE;
		m_respawnTime = npcCsv->RespawnTime;
		m_gold = npcCsv->GoldReward;
		m_chaseDistance = npcCsv->ChaseDistance;
		m_attackDistance = npcCsv->AttackDistance;

		m_lastHealTime = TimeUtil::CurTime();

		m_initBoundingBox.Center = { GameUtil::GetMonsterInitBB(m_npcType)->offset.x, GameUtil::GetMonsterInitBB(m_npcType)->offset.y,GameUtil::GetMonsterInitBB(m_npcType)->offset.z };
		m_initBoundingBox.Extents = { GameUtil::GetMonsterInitBB(m_npcType)->extent.x, GameUtil::GetMonsterInitBB(m_npcType)->extent.y, GameUtil::GetMonsterInitBB(m_npcType)->extent.z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMMATRIX mtxScale = DirectX::XMMatrixScaling(npcCsv->Scale, npcCsv->Scale, npcCsv->Scale);
		XMStoreFloat4x4(&m_worldMatrix, mtxScale * DirectX::XMLoadFloat4x4(&m_worldMatrix));
		m_worldMatrix._21 = 0.f; m_worldMatrix._22 = 1.f; m_worldMatrix._23 = 0.f;
	}

	void CMonster::RegisterKillBuff(int clientID)
	{
		const auto attackBuff = m_npcCsv->BuffInfos.find(EBuffType::AttackIncrease);
		const auto defenseBuff = m_npcCsv->BuffInfos.find(EBuffType::DefenseIncrease);
		const auto speedBuff = m_npcCsv->BuffInfos.find(EBuffType::SpeedIncrease);
		const auto utilBuff = m_npcCsv->BuffInfos.find(EBuffType::UtilIncrease);

		uint8_t attackValue = 0;
		uint8_t defenseValue = 0;
		float speedValue = 0.f;
		uint8_t utilValue = 0;
		uint16_t buffDuration = 0;
		if (attackBuff != m_npcCsv->BuffInfos.end()) {
			attackValue = static_cast<uint8_t>(attackBuff->second.buffValue);
			buffDuration = attackBuff->second.buffDuration;
		}
		if (defenseBuff != m_npcCsv->BuffInfos.end()) {
			defenseValue = static_cast<uint8_t>(defenseBuff->second.buffValue);
			buffDuration = defenseBuff->second.buffDuration;
		}
		if (speedBuff != m_npcCsv->BuffInfos.end()) {
			speedValue = static_cast<uint8_t>(speedBuff->second.buffValue);
			buffDuration = speedBuff->second.buffDuration;
		}
		if (utilBuff != m_npcCsv->BuffInfos.end()) {
			utilValue = static_cast<uint8_t>(utilBuff->second.buffValue);
			buffDuration = utilBuff->second.buffDuration;
		}


		//Player Buff
		CStat changeStat(0);
		changeStat.strength = attackValue;
		changeStat.magic = attackValue;
		changeStat.armor = defenseValue;
		changeStat.regist = defenseValue;
		changeStat.speed = speedValue;
		changeStat.endure = utilValue;
		changeStat.critical = utilValue;

		auto client = CObjectMgr::GetInstance()->GetClient(clientID);
		auto SetClientBuff = [this, &changeStat, &buffDuration](uint8_t _attackValue, uint8_t _defenseValue, float _speedValue, uint8_t _utilValue, int _clientID) {
			auto client = CObjectMgr::GetInstance()->GetClient(_clientID);
			CStat stat = client->GetStatus()->GetStat();
			stat.strength += _attackValue;
			stat.magic += _attackValue;
			stat.armor += _defenseValue;
			stat.regist += _defenseValue;
			stat.speed += _speedValue;
			stat.endure += _utilValue;
			stat.critical += _utilValue;
			client->GetStatus()->SetStat(stat);
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				std::shared_ptr<CClient> player = CObjectMgr::GetInstance()->GetClient(id);
				player->GetPacketSender()->SendMonsterKillBuffPacket(0, client->GetMatchId());
			}

			network::GetInstance()->RegisterTimerEvent({ client->GetID(), TimeUtil::PassedTimeMSec(buffDuration), EVENT_TYPE::EV_STAT_CHANGE, 0, changeStat });
		};

		if (client->GetMatchId() == 3) {
			SetClientBuff(attackValue, defenseValue, speedValue, utilValue, clientID);
		}
		else {
			//Heros
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				SetClientBuff(attackValue, defenseValue, speedValue, utilValue, id);
			}
		}
	}

	void CMonster::Move(float elapsedTime)
	{
		vec3 shift = { 0, 0, 0 };
		shift = vec3::Add(shift, m_targetLook, MINON_SPEED * m_speed);
		m_vel += shift;

		float velocity = sqrtf(m_vel.x * m_vel.x + m_vel.z * m_vel.z);
		if (velocity > m_maxVelXZ) {
			m_vel.x *= (m_maxVelXZ / velocity);
			m_vel.z *= (m_maxVelXZ / velocity);
		}

		shift = m_vel * elapsedTime;

		float height;
		if (GameUtil::NpcCollisionCheck(m_id, m_matchNum, shift)) {
			if (GameUtil::MapCollision(m_pos, shift, height, m_curNode)) {
				m_pos += shift;
				m_pos.y = height;
				UpdateBoundingBox();

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
				}
			}
			else {
				m_pos += shift;
				UpdateBoundingBox();

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
				}
			}
		}

		velocity = m_vel.Length();
		float deceleration = m_friction * elapsedTime;
		if (deceleration > velocity)
			deceleration = velocity;
		m_vel = vec3::Add(m_vel, vec3::Normalize(m_vel * -deceleration));
	}

	void CMonster::Heal()
	{
		m_hpLock.lock();
		m_curHp += m_maxHp * m_npcCsv->HealPercent;
		if (m_curHp > m_maxHp) {
			m_curHp = m_maxHp;
		}
		m_hpLock.unlock();

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendNpcStatChangePacket(m_id - NPC_ID, m_maxHp, m_curHp);
		}

		if ((m_state == NPC_STATE::ST_RETURN || m_state == NPC_STATE::ST_IDLE) && m_curHp < m_maxHp) {
			Heal();	// healCoolTime üũ m_npcCsv->HealCooltime			
		}

	}


	CUniqueRed::CUniqueRed()
	{
		m_npcType = ENpcType::RedDragon;
		__super::SetMonsterInfo();
	}

	bool CUniqueRed::Update(float elapsedTime)
	{
		if (NPC_STATE::ST_CHASE == m_state) {
			m_state = NPC_STATE::ST_ATTACK;
		}
		else if (NPC_STATE::ST_ATTACK == m_state) {
			if (-1 == m_targetClientID) {
				m_state = NPC_STATE::ST_IDLE;
				return true;
			}
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(m_targetClientID);
			m_targetPos.x = client->GetPos().x;
			m_targetPos.z = client->GetPos().z;
			LookTarget();
			Rotate(elapsedTime);
			UpdateBoundingBox();
			if (DistanceXZ(client->GetPos(), m_pos) < m_attackDistance) {
				Attack(elapsedTime);
			}
			else {
				m_state = NPC_STATE::ST_IDLE;
				m_targetPos.x = m_initLook.x;
				m_targetPos.z = m_initLook.z;
			}

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
			}
		}

		else {
			LookTarget();
			Rotate(elapsedTime);
			UpdateBoundingBox();
			if (TimeUtil::CurTime() - m_lastHealTime > std::chrono::milliseconds(m_npcCsv->HealCooltime)) {
				m_lastHealTime = TimeUtil::CurTime();
				m_hpLock.lock();
				m_curHp += static_cast<int>(m_maxHp * 0.2f);
				if (m_curHp > m_maxHp) {
					m_curHp = m_maxHp;
				}
				m_hpLock.unlock();
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendNpcStatChangePacket(m_id - NPC_ID, m_maxHp, m_curHp);
				}
			}

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
			}
		}

		return true;
	}

	bool CUniqueRed::Damaged(int clientID, int power, DAMAGE_TYPE type, bool updateTarget)
	{
		if (CMonster::Damaged(clientID, power, type, updateTarget)) {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientID);

			//Teleport Activation Check
			if (!CGameMgr::GetInstance()->GetTeleport(m_matchNum)) {
				CGameMgr::GetInstance()->SetTeleport(m_matchNum, true);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendTeleportActivePacket(true);
				}

			}

			return true;
		}
		return false;
	}

	////// Rare Monster ////// ////// Rare Monster //////

	CRareGreen::CRareGreen()
	{
		m_npcType = ENpcType::GreenDragon;
		__super::SetMonsterInfo();
	}

	CRareGolem::CRareGolem()
	{
		m_npcType = ENpcType::Golem;
		__super::SetMonsterInfo();
	}

	////// Normal Monster ////// ////// Normal Monster //////

	CNormalBear::CNormalBear()
	{
		m_npcType = ENpcType::Bear;
		__super::SetMonsterInfo();
	}

	CNormalMinotaur::CNormalMinotaur()
	{
		m_npcType = ENpcType::Minotaur;
		__super::SetMonsterInfo();
	}

	CNormalChest::CNormalChest()
	{
		m_npcType = ENpcType::Chest;
		__super::SetMonsterInfo();
	}

	CNormalBeholder::CNormalBeholder()
	{
		m_npcType = ENpcType::Beholder;
		__super::SetMonsterInfo();
	}
}