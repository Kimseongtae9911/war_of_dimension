#include "pch.h"
#include "GameObject.h"
#include "CNpc.h"
#include "CMinion.h"

namespace wod_server {
	CMinion::CMinion()
	{
		auto npcCsv = NpcCsvMgr::GetInstance()->GetNpcCsv(ENpcType::Minion);
		if (npcCsv == nullptr)
		{
			LogPrinter::PrintMsg("Minion Constructor Failed, npcCsv is not valid");
			return;
		}

		m_npcCsv = npcCsv;
		m_maxHp = m_curHp = static_cast<int>(m_npcCsv->BaseHp);
		m_maxVelXZ = m_npcCsv->MaxSpeed;
		m_pathCount = 0;
		m_path = 3;
		m_power = m_npcCsv->BaseAttack;

		m_initBoundingBox.Center = { GameUtil::GetMinionInitBB().m_offset.m_x, GameUtil::GetMinionInitBB().m_offset.m_y,GameUtil::GetMinionInitBB().m_offset.m_z };
		m_initBoundingBox.Extents = { GameUtil::GetMinionInitBB().m_extent.m_x, GameUtil::GetMinionInitBB().m_extent.m_y, GameUtil::GetMinionInitBB().m_extent.m_z };

		DirectX::XMStoreFloat4x4(&m_worldMatrix, DirectX::XMMatrixIdentity());
		DirectX::XMMATRIX mtxScale = DirectX::XMMatrixScaling(m_npcCsv->Scale, m_npcCsv->Scale, m_npcCsv->Scale);
		XMStoreFloat4x4(&m_worldMatrix, mtxScale * DirectX::XMLoadFloat4x4(&m_worldMatrix));
		m_worldMatrix._21 = 0.f; m_worldMatrix._22 = 1.f; m_worldMatrix._23 = 0.f;
	}

	CMinion::~CMinion()
	{
	}

	void CMinion::Initialize(uint8_t _posIndex)
	{
	}

	bool CMinion::Update(float _elapsedTime)
	{
		if (NPC_STATE::ST_MOVETO_STRUCTURE == m_state) {
			MoveToStructure(_elapsedTime);
		}
		else if (NPC_STATE::ST_ATTACK_STRUCTURE == m_state) {
			if (m_targetClientID == PATH_NUM) {
				if (AttackStructure()) {
					CGameMgr::GetInstance()->GetNexus(m_matchNum)->Damage(m_power);
				}
			}
			else {
				if (AttackStructure()) {
					CGameMgr::GetInstance()->GetTower(m_matchNum, m_targetClientID)->Damage(m_power);
					if (CGameMgr::GetInstance()->GetTower(m_matchNum, m_targetClientID)->GetBroken())
						m_state = NPC_STATE::ST_MOVE;
				}
			}
		}
		else if (NPC_STATE::ST_CHASE == m_state) {
			float distance = DistanceXZ(m_pos, CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos());
			if (distance >= m_npcCsv->ChaseMaxDistance) {
				m_state = NPC_STATE::ST_RETURN;
				ReturnToPath();
				return true;
			}
			else if (distance < m_npcCsv->AttackDistance) {
				m_state = NPC_STATE::ST_ATTACK;
				return true;
			}

			Chase(_elapsedTime);
		}
		else if (NPC_STATE::ST_RETURN == m_state) {
			Chase(_elapsedTime);
		}
		else if (NPC_STATE::ST_MOVE == m_state) {
			LookTarget();
			Move(_elapsedTime);
		}
		else if (NPC_STATE::ST_ATTACK == m_state) {
			Attack(_elapsedTime);
		}
		else {
			UpdateBoundingBox();
		}

		return true;
	}

	void CMinion::Move(float _elapsedTime)
	{
		if (m_rotate)
			Rotate(_elapsedTime);

		if (m_attack) {
			return;
		}

		vec3 shift = { 0, 0, 0 };
		shift = vec3::Add(shift, m_targetLook, MINON_SPEED * m_speed);
		m_vel += shift;

		float velocity = sqrtf(m_vel.m_x * m_vel.m_x + m_vel.m_z * m_vel.m_z);

		//Check the velocity is below the maximum velocity
		if (velocity > m_maxVelXZ) {
			m_vel.m_x *= (m_maxVelXZ / velocity);
			m_vel.m_z *= (m_maxVelXZ / velocity);
		}

		//Check collision and moves if it doesn't collide
		shift = m_vel * _elapsedTime;

		float height;
		if (GameUtil::NpcCollisionCheck(m_id, m_matchNum, shift)) {
			if (GameUtil::MapCollision(m_pos, shift, height, m_curNode)) {
				m_pos += shift;
				m_pos.m_y = height;
				UpdateBoundingBox();

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
					if (-1 == id)
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
		else {
			GameUtil::MapCollision(m_pos, height, m_curNode);
			m_pos.m_y = height;
			UpdateBoundingBox();

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMoveNpcPacket(m_id - NPC_ID, m_pos, m_look, m_right);
			}
		}


		//Check Client in Distance
		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (clientIDs[i] == -1)
				continue;
			if (DistanceXZ(m_pos, CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos()) < m_npcCsv->ChaseDistance) {
				if (GameUtil::GetNode(m_curNode)->m_triangle.m_id == GameUtil::GetNode(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetCurNode())->m_triangle.m_id) {
					m_state = NPC_STATE::ST_CHASE;
					m_targetClientID = clientIDs[i];
					break;
				}
				if (!Astar(GameUtil::GetNode(m_curNode), GameUtil::GetNode(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetCurNode()))) {
					break;
				}
				else {
					m_state = NPC_STATE::ST_CHASE;
				}
				break;
			}
		}

		//Check Nexus in Distance //todo: 미니언 + 건물 반지름을 통해 비교
		if (DistanceXZ(m_pos, CGameMgr::GetInstance()->GetNexus(m_matchNum)->GetPos()) < MOVETO_STRUCTURE_DISTANCE) {
			m_targetClientID = PATH_NUM;
			m_state = NPC_STATE::ST_MOVETO_STRUCTURE;
			return;
		}

		//Check Tower in Distance
		for (int i = 0; i < PATH_NUM; ++i) {
			if (CGameMgr::GetInstance()->GetTower(m_matchNum, i)->GetBroken())
				continue;
			//todo: 미니언 + 건물 반지름을 통해 비교
			if (DistanceXZ(m_pos, TOWER_POS[i]) < MOVETO_STRUCTURE_DISTANCE) {
				m_state = NPC_STATE::ST_MOVETO_STRUCTURE;
				m_targetClientID = i;
				return;
			}
		}

		if (m_state != NPC_STATE::ST_MOVETO_STRUCTURE && IsFloatEqual(m_pos.m_x, GameUtil::m_minionPaths[m_path][m_pathCount].m_x, 1.5f) && IsFloatEqual(m_pos.m_z, GameUtil::m_minionPaths[m_path][m_pathCount].m_z, 1.5f)) {
			m_pathCount++;
			if (m_pathCount >= GameUtil::m_minionPaths[m_path].size()) {
				m_state = NPC_STATE::ST_ATTACK_STRUCTURE;
				m_targetClientID = PATH_NUM;
				return;
			}
			m_targetPos = GameUtil::m_minionPaths[m_path][m_pathCount];
			LookTarget();
		}


		//Deceleration calculation
		velocity = m_vel.Length();
		float deceleration = m_friction * _elapsedTime;
		if (deceleration > velocity)
			deceleration = velocity;
		m_vel = vec3::Add(m_vel, vec3::Normalize(m_vel * -deceleration));
	}

	bool CMinion::AttackStructure()
	{
		if (!m_active)
			return false;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - m_attackTime) < std::chrono::milliseconds(m_npcCsv->AttackCooltime)) {
			m_state = NPC_STATE::ST_MOVETO_STRUCTURE;
			return false;
		}
		else {
			m_attack = true;
			m_attackTime = std::chrono::system_clock::now().time_since_epoch();
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendNpcAttackPacket(m_id - NPC_ID);
			}
			return true;
		}
	}

	void CMinion::Reset()
	{
		m_pathCount = 0;
		m_attack = false;
		m_targetClientID = -1;
		m_state = NPC_STATE::ST_MOVE;
		m_active = true;
	}

	void CMinion::Respawn(int _currTime)
	{
		int npcID = m_id - NPC_ID;

		float height = 0.f;
		int nodeNum = -1;

		const auto& respawnPos = m_npcCsv->m_respawnPos[CGameMgr::GetInstance()->GetPathNum(m_matchNum)];
		GameUtil::MapCollision(respawnPos, height, nodeNum);
		SetPos(respawnPos.m_x, height, respawnPos.m_z);
		UpdateBoundingBox();
		SetCurNode(nodeNum);

		//Set Minion Path
		SetTargetPos(GameUtil::m_minionPaths[CGameMgr::GetInstance()->GetPathNum(m_matchNum)][0]);
		SetPath(CGameMgr::GetInstance()->GetPathNum(m_matchNum));
		LookTarget();

		InitializeHp(m_npcCsv->BaseAttack + _currTime * m_npcCsv->HpIncrease);

		for (int clId : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (clId == -1)
				continue;

			CObjectMgr::GetInstance()->GetClient(clId)->GetPacketSender()->SendAddNpcPacket(npcID, GetPos(), GetLook(), GetRight());
			CObjectMgr::GetInstance()->GetClient(clId)->GetPacketSender()->SendNpcStatChangePacket(npcID, GetMaxHp(), GetCurHp());
		}

		Reset();
	}

	void CMinion::ReturnToPath()
	{
		int destNode = -1;
		switch (m_path) {
		case 0:
			destNode = 525;
			break;
		case 1:
			destNode = 1308;
			break;
		case 2:
			destNode = 1404;
			break;
		case 3:
			destNode = 1412;
			break;
		default:
			destNode = 1404;
			break;
		}

		if (!Astar(GameUtil::GetNode(m_curNode), GameUtil::GetNode(destNode))) {
			int pathCount = 0;
			float distance = FLT_MAX;
			for (int i = 0; i < PATH_NUM; ++i) {
				float newDistance = sqrtf(powf(m_pos.m_x - GameUtil::m_minionPaths[m_path][i].m_x, 2.f) + powf(m_pos.m_z - GameUtil::m_minionPaths[m_path][i].m_z, 2.f));
				if (newDistance < distance) {
					distance = newDistance;
					pathCount = i;
				}
			}
			if (pathCount < m_pathCount)
				pathCount = m_pathCount;
			else
				m_pathCount = pathCount;
			SetTargetPos(GameUtil::m_minionPaths[m_path][pathCount]);

			LookTarget();
			m_state = NPC_STATE::ST_MOVE;
		}
		else {
			m_state = NPC_STATE::ST_CHASE;
		}
	}

	void CMinion::MoveToStructure(float _elapsedTime)
	{
		if (!m_active) {
			m_state = NPC_STATE::ST_IDLE;
			return;
		}

		if (m_targetClientID == PATH_NUM) {
			CNexus* nexus = CGameMgr::GetInstance()->GetNexus(m_matchNum);
			m_targetPos.m_x = nexus->GetPos().m_x;
			m_targetPos.m_z = nexus->GetPos().m_z;

			//todo: 미니언 + 넥서스 반지름 + 미니언 공격 사거리를 통해 비교
			if (DistanceXYZ(m_pos, nexus->GetPos()) < NEXUS_ATTACK_DISTANCE) {
				m_state = NPC_STATE::ST_ATTACK_STRUCTURE;
				LookTarget();
				return;
			}

		}
		else {
			CTower* tower = CGameMgr::GetInstance()->GetTower(m_matchNum, m_targetClientID);
			m_targetPos.m_x = tower->GetPos().m_x;
			m_targetPos.m_z = tower->GetPos().m_z;

			//todo: 미니언 + 타워 반지름 + 미니언 공격 사거리를 통해 비교
			if (DistanceXYZ(m_pos, tower->GetPos()) < TOWER_ATTACK_DISTANCE) {
				m_state = NPC_STATE::ST_ATTACK_STRUCTURE;
				LookTarget();
				return;
			}
			else if (tower->GetBroken())
				m_state = NPC_STATE::ST_MOVE;
		}

		LookTarget();
		Move(_elapsedTime);
	}
}