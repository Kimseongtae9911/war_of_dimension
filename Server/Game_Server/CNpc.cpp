#include "pch.h"
#include "GameObject.h"
#include "CNpc.h"
#include "CClient.h"

namespace wod_server {

	CNpc::CNpc()
	{
		m_look = { 0.0f, 0.0f, 1.0f };
		m_up = { 0.0f, 1.0f, 0.0f };
		m_right = { 1.0f, 0.0f, 0.0f };
		m_speed = 1.0f;
		m_state = NPC_STATE::ST_MOVE;
		m_friction = WORLD_FRICTION;
		m_attackTime = std::chrono::system_clock::now().time_since_epoch();
		m_targetLook = { 0.0f, 0.0f, 0.0f };
		m_targetClientID = -1;
	}

	CNpc::~CNpc()
	{
	}

	void CNpc::Chase(float _elapsedTime)
	{
		if (m_attack) {
			LookTarget();
			Rotate(_elapsedTime);
			return;
		}

		if (!m_chasePath.empty()) {
			Node* nextNode = m_chasePath.top();

			vec3 centroid = vec3( (nextNode->m_triangle.m_v1.m_x + nextNode->m_triangle.m_v2.m_x + nextNode->m_triangle.m_v3.m_x) / 3.0f,
								  (nextNode->m_triangle.m_v1.m_y + nextNode->m_triangle.m_v2.m_y + nextNode->m_triangle.m_v3.m_y) / 3.0f,
								  (nextNode->m_triangle.m_v1.m_z + nextNode->m_triangle.m_v2.m_z + nextNode->m_triangle.m_v3.m_z) / 3.0f);

			m_targetPos.m_x = centroid.m_x;
			m_targetPos.m_z = centroid.m_z;
			LookTarget();

			if (IsFloatEqual(m_pos.m_x, m_targetPos.m_x, 1.0f) && IsFloatEqual(m_pos.m_z, m_targetPos.m_z, 1.0f)) {
				m_chasePath.pop();
			}
		}
		else {
			if (m_targetClientID == -1)
				return;
			vec3 clientPos = CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos();
			m_targetPos.m_x = clientPos.m_x;
			m_targetPos.m_z = clientPos.m_z;
			LookTarget();
		}

		if (m_rotate)
			Rotate(_elapsedTime);

		Move(_elapsedTime);
	}

	void CNpc::Attack(float _elapsedTime)
	{
		if (!m_active)
			return;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - m_attackTime) < std::chrono::milliseconds(m_npcCsv->AttackCooltime) || m_attack) {
			m_state = NPC_STATE::ST_CHASE;
		}
		else {
			LookTarget();
			Rotate(_elapsedTime);
			m_attack = true;
			m_attackTime = std::chrono::system_clock::now().time_since_epoch();
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendNpcAttackPacket(m_id - NPC_ID);
			}
			CObjectMgr::GetInstance()->GetClient(m_targetClientID)->Damage(m_power, 0, DAMAGE_TYPE::STRENGTH, m_id);

			if (CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetStatus()->m_healthMana.GetCurHp() <= 0)
				m_state = NPC_STATE::ST_RETURN;
		}
	}

	void CNpc::Rotate(float _elapsedTime)
	{
		if (m_look != m_targetLook) {
			vec3 newLook = vec3::Normalize(vec3::Lerp(m_look, m_targetLook, m_rotateSpeed * _elapsedTime));

			if (newLook.Dot(m_targetLook) >= 1.0f - FLT_EPSILON) {
				m_look = m_targetLook;
				m_rotate = false;
			}
			else {
				m_look = newLook;
			}
			m_right = vec3::Normalize(m_up.Cross(m_look));
		}
	}

	bool CNpc::Damaged(int _clientID, int _power, DAMAGE_TYPE _type, bool _updateTarget)
	{
		m_hpLock.lock();
		m_curHp -= _power;
		//Remove Npc
		if (m_curHp <= 0) {
			m_hpLock.unlock();
			m_active = false;
			m_state = NPC_STATE::ST_IDLE;
			m_attack = false;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
				client->GetPacketSender()->SendRemoveNpcPacket(m_id - NPC_ID, GetPacketType());
			}
			return true;
		}
		else
			m_hpLock.unlock();

		//Damage Npc
		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
			if (-1 == id)
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
			client->GetPacketSender()->SendNpcStatChangePacket(m_id - NPC_ID, m_maxHp, m_curHp);
		}
		return false;
	}

	void CNpc::LookTarget()
	{
		vec3 target = { m_targetPos.m_x, 0.0f, m_targetPos.m_z };
		m_targetLook = target - m_pos;
		m_targetLook.m_y = 0.f;
		m_targetLook = vec3::Normalize(m_targetLook);

		if (m_look != m_targetLook)
			m_rotate = true;
	}

	void CNpc::UpdateBoundingBox()
	{
		m_worldMatrix._11 = m_right.m_x; m_worldMatrix._12 = m_right.m_y; m_worldMatrix._13 = m_right.m_z;
		m_worldMatrix._31 = m_targetLook.m_x; m_worldMatrix._32 = m_targetLook.m_y; m_worldMatrix._33 = m_targetLook.m_z;
		m_worldMatrix._41 = m_pos.m_x; m_worldMatrix._42 = m_pos.m_y; m_worldMatrix._43 = m_pos.m_z;

		if (-1 == m_targetClientID) {
			m_worldMatrix._31 = m_look.m_x; m_worldMatrix._32 = m_look.m_y; m_worldMatrix._33 = m_look.m_z;
		}

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	bool CNpc::Astar(Node* _startNode, Node* _endNode)
	{
		while (!m_chasePath.empty()) {
			m_chasePath.pop();
		}

		std::unordered_set<Node*> visitedNodes;

		std::priority_queue < PathNode*, std::vector<PathNode*>, decltype([](const PathNode* _a, const PathNode* _b) {
			return _a->m_fScore > _b->m_fScore;
			})> openSet;

		openSet.push(new PathNode(_startNode, 0.0f, CalculateHeuristic(_startNode, _endNode)));

		std::unordered_map<Node*, Node*> cameFrom;
		std::unordered_map<Node*, float> gScore;

		gScore[_startNode] = 0.0f;

		while (!openSet.empty())
		{
			PathNode* currentPathNode = openSet.top();
			Node* currentNode = currentPathNode->m_node;
			openSet.pop();

			if (currentNode == _endNode) {
				Node* pathNode = currentNode;
				while (pathNode != nullptr) {
					m_chasePath.push(pathNode);
					pathNode = cameFrom[pathNode];
				}

				while (!openSet.empty()) {
					delete openSet.top();
					openSet.pop();
				}
				return true;
			}

			visitedNodes.insert(currentNode);

			for (Node* adjacentNode : currentNode->m_adjacentNodes) {
				if (visitedNodes.contains(adjacentNode))
					continue;

				float tempGScore = gScore[currentNode] + DistanceXYZ(currentNode, adjacentNode);

				if (tempGScore < gScore[adjacentNode]) {
					cameFrom[adjacentNode] = currentNode;
					gScore[adjacentNode] = tempGScore;
					float fScore = tempGScore + CalculateHeuristic(adjacentNode, _endNode);
					openSet.push(new PathNode(adjacentNode, tempGScore, fScore));
				}
			}
		}

		// No path found
		return false;
	}

	float CNpc::CalculateHeuristic(Node* _node, Node* _endNode)
	{
		vec3 centroid1 = vec3((_node->m_triangle.m_v1.m_x + _node->m_triangle.m_v2.m_x + _node->m_triangle.m_v3.m_x) / 3.0f,
							  (_node->m_triangle.m_v1.m_y + _node->m_triangle.m_v2.m_y + _node->m_triangle.m_v3.m_y) / 3.0f,
							  (_node->m_triangle.m_v1.m_z + _node->m_triangle.m_v2.m_z + _node->m_triangle.m_v3.m_z) / 3.0f );

		vec3 centroid2 = vec3((_endNode->m_triangle.m_v1.m_x + _endNode->m_triangle.m_v2.m_x + _endNode->m_triangle.m_v3.m_x) / 3.0f,
							  (_endNode->m_triangle.m_v1.m_y + _endNode->m_triangle.m_v2.m_y + _endNode->m_triangle.m_v3.m_y) / 3.0f,
							  (_endNode->m_triangle.m_v1.m_z + _endNode->m_triangle.m_v2.m_z + _endNode->m_triangle.m_v3.m_z) / 3.0f);


		float distance = centroid1.Magnitude(centroid2);

		return distance;
	}
}
