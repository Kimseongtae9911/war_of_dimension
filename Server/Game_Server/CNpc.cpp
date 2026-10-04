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

	void CNpc::Chase(float elapsedTime)
	{
		if (m_attack) {
			LookTarget();
			Rotate(elapsedTime);
			return;
		}

		if (!m_chasePath.empty()) {
			Node* nextNode = m_chasePath.top();

			vec3 centroid = vec3( (nextNode->triangle.v1.x + nextNode->triangle.v2.x + nextNode->triangle.v3.x) / 3.0f,
								  (nextNode->triangle.v1.y + nextNode->triangle.v2.y + nextNode->triangle.v3.y) / 3.0f,
								  (nextNode->triangle.v1.z + nextNode->triangle.v2.z + nextNode->triangle.v3.z) / 3.0f);

			m_targetPos.x = centroid.x;
			m_targetPos.z = centroid.z;
			LookTarget();

			if (IsFloatEqual(m_pos.x, m_targetPos.x, 1.0f) && IsFloatEqual(m_pos.z, m_targetPos.z, 1.0f)) {
				m_chasePath.pop();
			}
		}
		else {
			if (m_targetClientID == -1)
				return;
			vec3 clientPos = CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetPos();
			m_targetPos.x = clientPos.x;
			m_targetPos.z = clientPos.z;
			LookTarget();
		}

		if (m_rotate)
			Rotate(elapsedTime);

		Move(elapsedTime);
	}

	void CNpc::Attack(float elapsedTime)
	{
		if (!active)
			return;

		if (std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch() - m_attackTime) < std::chrono::milliseconds(m_npcCsv->AttackCooltime) || m_attack) {
			m_state = NPC_STATE::ST_CHASE;
		}
		else {
			LookTarget();
			Rotate(elapsedTime);
			m_attack = true;
			m_attackTime = std::chrono::system_clock::now().time_since_epoch();
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendNpcAttackPacket(m_id - NPC_ID);
			}
			CObjectMgr::GetInstance()->GetClient(m_targetClientID)->Damage(m_power, 0, DAMAGE_TYPE::STRENGTH, m_id);

			if (CObjectMgr::GetInstance()->GetClient(m_targetClientID)->GetStatus()->healthMana.GetCurHp() <= 0)
				m_state = NPC_STATE::ST_RETURN;
		}
	}

	void CNpc::Rotate(float elapsedTime)
	{
		if (m_look != m_targetLook) {
			vec3 newLook = vec3::Normalize(vec3::Lerp(m_look, m_targetLook, m_rotateSpeed * elapsedTime));

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

	bool CNpc::Damaged(int clientID, int power, DAMAGE_TYPE type, bool updateTarget)
	{
		m_hpLock.lock();
		m_curHp -= power;
		//Remove Npc
		if (m_curHp <= 0) {
			m_hpLock.unlock();
			active = false;
			m_state = NPC_STATE::ST_IDLE;
			m_attack = false;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
				if (-1 == id)
					continue;
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
				client->GetPacketSender()->SendRemoveNpcPacket(m_id - NPC_ID);
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
		vec3 target = { m_targetPos.x, 0.0f, m_targetPos.z };
		m_targetLook = target - m_pos;
		m_targetLook.y = 0.f;
		m_targetLook = vec3::Normalize(m_targetLook);

		if (m_look != m_targetLook)
			m_rotate = true;
	}

	void CNpc::UpdateBoundingBox()
	{
		m_worldMatrix._11 = m_right.x; m_worldMatrix._12 = m_right.y; m_worldMatrix._13 = m_right.z;
		m_worldMatrix._31 = m_targetLook.x; m_worldMatrix._32 = m_targetLook.y; m_worldMatrix._33 = m_targetLook.z;
		m_worldMatrix._41 = m_pos.x; m_worldMatrix._42 = m_pos.y; m_worldMatrix._43 = m_pos.z;

		if (-1 == m_targetClientID) {
			m_worldMatrix._31 = m_look.x; m_worldMatrix._32 = m_look.y; m_worldMatrix._33 = m_look.z;
		}

		m_initBoundingBox.Transform(m_boundingBox, DirectX::XMLoadFloat4x4(&m_worldMatrix));
	}

	bool CNpc::Astar(Node* startNode, Node* endNode)
	{
		while (!m_chasePath.empty()) {
			m_chasePath.pop();
		}

		std::unordered_set<Node*> visitedNodes;

		std::priority_queue < PathNode*, std::vector<PathNode*>, decltype([](const PathNode* a, const PathNode* b) {
			return a->fScore > b->fScore;
			})> openSet;

		openSet.push(new PathNode(startNode, 0.0f, CalculateHeuristic(startNode, endNode)));

		std::unordered_map<Node*, Node*> cameFrom;
		std::unordered_map<Node*, float> gScore;

		gScore[startNode] = 0.0f;

		while (!openSet.empty())
		{
			PathNode* currentPathNode = openSet.top();
			Node* currentNode = currentPathNode->node;
			openSet.pop();

			if (currentNode == endNode) {
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

			for (Node* adjacentNode : currentNode->adjacentNodes) {
				if (visitedNodes.contains(adjacentNode))
					continue;

				float tempGScore = gScore[currentNode] + DistanceXYZ(currentNode, adjacentNode);

				if (tempGScore < gScore[adjacentNode]) {
					cameFrom[adjacentNode] = currentNode;
					gScore[adjacentNode] = tempGScore;
					float fScore = tempGScore + CalculateHeuristic(adjacentNode, endNode);
					openSet.push(new PathNode(adjacentNode, tempGScore, fScore));
				}
			}
		}

		// No path found
		return false;
	}

	float CNpc::CalculateHeuristic(Node* node, Node* endNode)
	{
		vec3 centroid1 = vec3((node->triangle.v1.x + node->triangle.v2.x + node->triangle.v3.x) / 3.0f, 
							  (node->triangle.v1.y + node->triangle.v2.y + node->triangle.v3.y) / 3.0f, 
							  (node->triangle.v1.z + node->triangle.v2.z + node->triangle.v3.z) / 3.0f );

		vec3 centroid2 = vec3((endNode->triangle.v1.x + endNode->triangle.v2.x + endNode->triangle.v3.x) / 3.0f,
							  (endNode->triangle.v1.y + endNode->triangle.v2.y + endNode->triangle.v3.y) / 3.0f,
							  (endNode->triangle.v1.z + endNode->triangle.v2.z + endNode->triangle.v3.z) / 3.0f);


		float distance = centroid1.Magnitude(centroid2);

		return distance;
	}
}