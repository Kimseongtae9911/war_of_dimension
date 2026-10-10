#include "pch.h"
#include "CClient.h"
#include "CNpc.h"
#include <fstream>
#include <sstream>

namespace wod_server {
	std::array<std::unordered_map<int, int>, MAX_ROLE> GameUtil::m_playerSkillCoolTimes;
	std::array<std::unordered_map<int, int>, MAX_ROLE> GameUtil::m_playerManaConsumption;
	std::vector<Node*> GameUtil::m_naviMesh;
	std::vector<Triangle> GameUtil::m_heightMesh;
	std::array<std::array<std::vector<DirectX::BoundingOrientedBox>, SECTION_NUM>, SECTION_NUM> GameUtil::m_boundingBoxs;
	InitBoundingBox GameUtil::m_playerInitBB;
	InitBoundingBox GameUtil::m_bossPlayerInitBB;
	InitBoundingBox GameUtil::m_minionInitBB;
	std::map<ENpcType, InitBoundingBox*> GameUtil::m_monsterInitBB;
	std::array<std::vector<vec2>, PATH_NUM> GameUtil::m_minionPaths = { {} };
	std::vector<DirectX::BoundingOrientedBox> GameUtil::m_magneticFences;
	DirectX::BoundingOrientedBox GameUtil::m_nexusBB;
	std::array<DirectX::BoundingOrientedBox, PATH_NUM> GameUtil::m_towerBBs;

	GameUtil::~GameUtil()
	{
		for (Node* node : m_naviMesh) {
			delete node;
		}

		for (auto& [key, bb] : m_monsterInitBB)
			delete bb;
	}

	bool GameUtil::LoadCoolTime(std::string _filename)
	{
		std::fstream in(_filename);

		if (in.fail())
			return false;

		while (!in.eof()) {
			std::string str;
			in >> str;
			if (str == "#") {
				in >> str;
			}
			else {
				int characterNum = atoi(str.c_str());
				int skillCount;
				in >> skillCount;
				in >> str;	// Character name

				int skill; int cooltime;
				for (int i = 0; i < skillCount; ++i) {
					in >> skill;
					in >> cooltime;
					in >> str;	// Skill name
					m_playerSkillCoolTimes[characterNum].insert({ skill, cooltime });
				}
			}
		}

		return true;
	}

	bool GameUtil::LoadManaConsumption(std::string _filename)
	{
		std::fstream in(_filename);

		if (in.fail())
			return false;

		while (!in.eof()) {
			std::string str;
			in >> str;
			if (str == "#") {
				in >> str;
			}
			else {
				int characterNum = atoi(str.c_str());
				int skillCount;
				in >> skillCount;
				in >> str;	// Character name

				int skill; int cooltime;
				for (int i = 0; i < skillCount; ++i) {
					in >> skill;
					in >> cooltime;
					in >> str;	// Skill name
					m_playerManaConsumption[characterNum].insert({ skill, cooltime });
				}
			}
		}

		return true;
	}

	bool GameUtil::LoadNaviMesh(std::string _filename)
	{
		std::ifstream in(_filename);
		if (in.fail())
			return false;

		std::vector<vec3> vertices;
		int triangleID = 0;

		std::string line;
		while (std::getline(in, line)) {
			std::stringstream ss(line);
			std::string token;
			ss >> token;

			if (token == "v") {
				vec3 vertex;
				ss >> vertex.m_x >> vertex.m_y >> vertex.m_z;
				vertices.push_back(vertex);
			}
			else if (token == "f") {
				int index1, index2, index3;
				ss >> index1 >> index2 >> index3;
				Triangle triangle(vertices[index1 - 1], vertices[index2 - 1], vertices[index3 - 1], triangleID++);

				Node* currentNodePtr = new Node(triangle);
				m_naviMesh.push_back(currentNodePtr);

				//Check Adjacent Triangle
				for (const auto& node : m_naviMesh)
				{
					if (&node->m_triangle != &currentNodePtr->m_triangle && CheckTriangleAdjacent(node->m_triangle, currentNodePtr->m_triangle))
					{
						currentNodePtr->m_adjacentNodes.push_back(node);
						node->m_adjacentNodes.push_back(currentNodePtr);
					}
				}
			}

		}

		return true;
	}

	bool GameUtil::LoadHeightMesh(std::string _filename)
	{
		std::ifstream in(_filename);
		if (in.fail())
			return false;

		std::vector<vec3> vertices;

		std::string line;
		while (std::getline(in, line)) {
			std::stringstream ss(line);
			std::string token;
			ss >> token;

			if (token == "v") {
				vec3 vertex;
				ss >> vertex.m_x >> vertex.m_y >> vertex.m_z;
				vertices.push_back(vertex);
			}
			else if (token == "f") {
				int index1, index2, index3;
				ss >> index1 >> index2 >> index3;
				m_heightMesh.push_back(Triangle(vertices[index1 - 1], vertices[index2 - 1], vertices[index3 - 1]));
			}
		}

		std::sort(m_heightMesh.begin(), m_heightMesh.end(), [](const Triangle& _t1, const Triangle& _t2) {
			return (_t1.m_v1.m_y + _t1.m_v2.m_y + _t1.m_v3.m_y) / 3.f > (_t2.m_v1.m_y + _t2.m_v2.m_y + _t2.m_v3.m_y) / 3.f;
			});

		return true;
	}

	bool GameUtil::LoadMap(std::string _filename)
	{
		std::fstream in(_filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		in >> boxCnt;

		int objectNum;
		float center[3];
		float extent[3];
		float position[3];
		float quaternion[4];
		float scale[3];
		for (int i = 0; i < boxCnt; ++i) {
			//objectNum / Center / Extent / Position / Quaternion / Scale
			in >> objectNum;
			for (int i = 0; i < 3; ++i)
				in >> center[i];
			for (int i = 0; i < 3; ++i)
				in >> extent[i];
			for (int i = 0; i < 3; ++i)
				in >> position[i];
			for (int i = 0; i < 4; ++i)
				in >> quaternion[i];
			for (int i = 0; i < 3; ++i)
				in >> scale[i];

			DirectX::BoundingOrientedBox obb;
			obb.CreateFromBoundingBox(obb, DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f)));
			DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
			DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
			obb.Transform(obb, transformMatrix);
			obb.Center = { center[0], center[1], center[2] };
			//Map pos (-200, -200) ~ (100, 100) so, need to add 200
			m_boundingBoxs[static_cast<int>((center[0] + 200) / (WORLD_WIDTH / SECTION_NUM))][static_cast<int>((center[2] + 200) / (WORLD_HEIGHT / SECTION_NUM))].push_back(obb);
		}

		return true;
	}

	bool GameUtil::LoadPlayerBB(std::string _filename, int _type)
	{
		std::fstream in(_filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		int objectNum = -1;
		in >> boxCnt;

		in >> objectNum;

		//0:HeroPlayer, 1:BossPlayer
		if (_type == 0) {
			in >> m_playerInitBB.m_offset.m_x;
			in >> m_playerInitBB.m_offset.m_y;
			in >> m_playerInitBB.m_offset.m_z;

			in >> m_playerInitBB.m_extent.m_x;
			in >> m_playerInitBB.m_extent.m_y;
			in >> m_playerInitBB.m_extent.m_z;
		}
		else {
			in >> m_bossPlayerInitBB.m_offset.m_x;
			in >> m_bossPlayerInitBB.m_offset.m_y;
			in >> m_bossPlayerInitBB.m_offset.m_z;

			in >> m_bossPlayerInitBB.m_extent.m_x;
			in >> m_bossPlayerInitBB.m_extent.m_y;
			in >> m_bossPlayerInitBB.m_extent.m_z;
		}
		return true;
	}

	bool GameUtil::LoadMinionBB(std::string _filename)
	{
		std::fstream in(_filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		int objectNum = -1;
		in >> boxCnt;

		in >> objectNum;

		in >> m_minionInitBB.m_offset.m_x;
		in >> m_minionInitBB.m_offset.m_y;
		in >> m_minionInitBB.m_offset.m_z;

		in >> m_minionInitBB.m_extent.m_x;
		in >> m_minionInitBB.m_extent.m_y;
		in >> m_minionInitBB.m_extent.m_z;

		return true;
	}

	bool GameUtil::LoadFenceBB(std::string _filename)
	{
		std::fstream in(_filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		in >> boxCnt;

		int objectNum;
		float center[3];
		float extent[3];
		float position[3];
		float quaternion[4];
		float scale[3];
		for (int i = 0; i < boxCnt; ++i) {
			//objectNum / Center / Extent / Position / Quaternion / Scale
			in >> objectNum;
			for (int i = 0; i < 3; ++i)
				in >> center[i];
			for (int i = 0; i < 3; ++i)
				in >> extent[i];
			for (int i = 0; i < 3; ++i)
				in >> position[i];
			for (int i = 0; i < 4; ++i)
				in >> quaternion[i];
			for (int i = 0; i < 3; ++i)
				in >> scale[i];

			DirectX::BoundingOrientedBox obb;
			obb.CreateFromBoundingBox(obb, DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.5f, 0.5f, 0.5f)));
			DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
			DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
			obb.Transform(obb, transformMatrix);
			obb.Center = { center[0], center[1], center[2] };

			m_magneticFences.push_back(obb);
		}

		return true;
	}

	bool GameUtil::LoadMinionPath(std::string _filename, int _pathNum)
	{
		std::fstream in(_filename);

		if (in.fail())
			return false;

		char c;
		float x, z;
		while (!in.eof()) {
			in >> c;
			in >> x >> z;

			m_minionPaths[_pathNum].push_back({ x, z });
		}
		return true;
	}

	bool GameUtil::LoadMonsterBB()
	{
		//std::fstream in("Resource/RedBB.txt");

		std::pair<std::fstream, ENpcType> in[7];
		in[0].first.open("Resource/RedBB.txt");
		in[0].second = ENpcType::RedDragon;
		in[1].first.open("Resource/GreenBB.txt");
		in[1].second = ENpcType::GreenDragon;
		in[2].first.open("Resource/GolemBB.txt");
		in[2].second = ENpcType::Golem;
		in[3].first.open("Resource/BearBB.txt");
		in[3].second = ENpcType::Bear;
		in[4].first.open("Resource/MinotaurBB.txt");
		in[4].second = ENpcType::Minotaur;
		in[5].first.open("Resource/ChestBB.txt");
		in[5].second = ENpcType::Chest;
		in[6].first.open("Resource/BeholderBB.txt");
		in[6].second = ENpcType::Beholder;

		for (int i = 0; i < 7; ++i) {
			if (in[i].first.fail()) {
				LogPrinter::PrintMsg("Failed To Open BB File" + i);
				return false;
			}
			InitBoundingBox* temp = new InitBoundingBox();

			int boxCnt = 0;
			int objectNum = -1;
			in[i].first >> boxCnt;

			in[i].first >> objectNum;

			in[i].first >> temp->m_offset.m_x;
			in[i].first >> temp->m_offset.m_y;
			in[i].first >> temp->m_offset.m_z;

			in[i].first >> temp->m_extent.m_x;
			in[i].first >> temp->m_extent.m_y;
			in[i].first >> temp->m_extent.m_z;

			m_monsterInitBB.emplace(in[i].second, temp);
		}
		return true;
	}

	bool GameUtil::LoadTowerBB()
	{
		std::fstream in[PATH_NUM];
		in[0].open("Resource/TowerBB0.txt");
		in[1].open("Resource/TowerBB1.txt");
		in[2].open("Resource/TowerBB2.txt");
		in[3].open("Resource/TowerBB3.txt");


		for (int i = 0; i < PATH_NUM; ++i) {
			if (in[i].fail())
				return false;

			int boxCnt = 0;
			in[i] >> boxCnt;

			int objectNum;
			float center[3];
			float extent[3];
			float position[3];
			float quaternion[4];
			float scale[3];
			for (int j = 0; j < boxCnt; ++j) {
				//objectNum / Center / Extent / Position / Quaternion / Scale
				in[i] >> objectNum;
				for (int k = 0; k < 3; ++k)
					in[i] >> center[k];
				for (int k = 0; k < 3; ++k)
					in[i] >> extent[k];
				for (int k = 0; k < 3; ++k)
					in[i] >> position[k];
				for (int k = 0; k < 4; ++k)
					in[i] >> quaternion[k];
				for (int k = 0; k < 3; ++k)
					in[i] >> scale[k];

				m_towerBBs[i].CreateFromBoundingBox(m_towerBBs[i], DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.25f, 0.25f, 0.25f)));
				DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
				DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
				DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
				m_towerBBs[i].Transform(m_towerBBs[i], transformMatrix);
				m_towerBBs[i].Center = { center[0], center[1], center[2] };
			}
		}

		return true;
	}

	bool GameUtil::LoadNexusBB(const std::string& _filename)
	{
		std::fstream in(_filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		in >> boxCnt;

		int objectNum;
		float center[3];
		float extent[3];
		float position[3];
		float quaternion[4];
		float scale[3];
		for (int i = 0; i < boxCnt; ++i) {
			//objectNum / Center / Extent / Position / Quaternion / Scale
			in >> objectNum;
			for (int j = 0; j < 3; ++j)
				in >> center[j];
			for (int j = 0; j < 3; ++j)
				in >> extent[j];
			for (int j = 0; j < 3; ++j)
				in >> position[j];
			for (int j = 0; j < 4; ++j)
				in >> quaternion[j];
			for (int j = 0; j < 3; ++j)
				in >> scale[j];

			m_nexusBB.CreateFromBoundingBox(m_nexusBB, DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.25f, 0.25f, 0.25f)));
			DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
			DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
			m_nexusBB.Transform(m_nexusBB, transformMatrix);
			m_nexusBB.Center = { center[0], center[1], center[2] };
		}

		return true;
	}

	int GameUtil::GetCoolTime(int _characterNum, int _skillNum)
	{
		if (m_playerSkillCoolTimes[_characterNum].contains(_skillNum))
			return m_playerSkillCoolTimes[_characterNum][_skillNum];
		else {
			LogPrinter::PrintMsg("Failed To Return CoolTime " + std::to_string(_skillNum));
			return 0;
		}
	}

	int GameUtil::GetMpConsumption(int _characterNum, int _skillNum)
	{
		if (m_playerManaConsumption[_characterNum].contains(_skillNum))
			return m_playerManaConsumption[_characterNum][_skillNum];
		else {
			LogPrinter::PrintMsg("Failed To Return Mp Consumption " + std::to_string(_skillNum));
			return 0;
		}
	}

	bool GameUtil::MapCollision(const vec3& _pos, float& _height, int& _nodeNum)
	{
		Ray ray;
		ray.m_origin = _pos;
		ray.m_origin.m_y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, _height, _nodeNum)) {
			ray.RayCast(m_heightMesh, _height);
			return true;
		}

		return false;
	}

	bool GameUtil::MapCollision(const vec3& _pos, vec3& _shift, float& _height, int& _nodeNum)
	{
		Ray ray;
		ray.m_origin = _pos + _shift;
		ray.m_origin.m_y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, _height, _nodeNum)) {
			ray.RayCast(m_heightMesh, _height);
			return true;
		}
		else {
			vec3 slidingVector;
			if (GetSlidingVector(_pos, _shift, _nodeNum, slidingVector)) {
				_shift = slidingVector * _shift.Length();
				ray.m_origin = _pos + _shift;
				ray.m_origin.m_y += RAY_OFFSET;
				if (ray.RayCast(m_naviMesh, _height, _nodeNum)) {
					return false;
				}
				else {
					float desiredMagnitude = slidingVector.Length() * 2.0f;
					slidingVector = vec3::Normalize(slidingVector) * desiredMagnitude;
					vec3 temp = _shift;
					_shift = slidingVector * _shift.Length() - temp;
					return false;
				}
			}
			else {
				std::cout << "Failed To Get Sliding Vector" << std::endl;
			}
		}
		return false;
	}

	bool GameUtil::MapCollision(const vec3& _pos, float& _height, int& _nodeNum, bool _skill)
	{
		Ray ray;
		ray.m_origin = _pos;
		ray.m_origin.m_y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, _height, _nodeNum, _skill)) {
			ray.RayCast(m_heightMesh, _height, _skill);
			return true;
		}
		return false;
	}

	bool GameUtil::FenceCollision(const DirectX::BoundingOrientedBox _box, const vec3& _shift)
	{
		DirectX::BoundingOrientedBox clientOBB = _box;
		clientOBB.Center.x += _shift.m_x; clientOBB.Center.y += _shift.m_y; clientOBB.Center.z += _shift.m_z;

		for (const auto& fence : m_magneticFences) {
			if (true == fence.Intersects(clientOBB))
				return true;
		}
		return false;
	}

	bool GameUtil::ClientCollisionCheck(int _id, const vec3& _shift)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_id);

		DirectX::BoundingOrientedBox clientOBB = client->GetBoundingBox();
		clientOBB.Center.x += _shift.m_x; clientOBB.Center.y += _shift.m_y; clientOBB.Center.z += _shift.m_z;

		for (int clID : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
			if (clID == -1 || clID == _id)
				continue;

			if (clientOBB.Intersects(CObjectMgr::GetInstance()->GetClient(clID)->GetBoundingBox())) {
				return false;
			}
		}

		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);

			if (!npc->m_active)
				continue;

			if (clientOBB.Intersects(npc->GetBoundingBox()))
				return false;
		}


		return true;
	}

	bool GameUtil::NpcCollisionCheck(int _id, int _matchNum, const vec3& _shift)
	{
		std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_matchNum, _id - NPC_ID);

		DirectX::BoundingOrientedBox npcOBB = npc->GetBoundingBox();
		npcOBB.Center.x += _shift.m_x; npcOBB.Center.y += _shift.m_y; npcOBB.Center.z += _shift.m_z;

		for (int clID : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
			if (clID == -1)
				continue;

			if (npcOBB.Intersects(CObjectMgr::GetInstance()->GetClient(clID)->GetBoundingBox())) {
				return false;
			}
		}

		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> otherNpc = CObjectMgr::GetInstance()->GetNpc(_matchNum, i);

			if (!otherNpc->m_active)
				continue;

			if (otherNpc->GetID() == _id)
				continue;

			if (npcOBB.Intersects(otherNpc->GetBoundingBox())) {
				//Calculate and Apply Sliding Vector
				vec3 npcPos = npc->GetPos();
				vec3 newPosition = npcPos + vec3::Reflect(_shift * 2.0f, vec3::Normalize(npcPos - otherNpc->GetPos()));
				npc->SetPos(newPosition);
				return false;
			}
		}


		return true;
	}

	bool GameUtil::HeroSkillCollisionCheck(const DirectX::BoundingOrientedBox _skillBox, int _matchNum, int _power, int _critical, DAMAGE_TYPE _type, int _clientID, bool _projectile)
	{
		if (_skillBox.Center.x < -200.f || _skillBox.Center.x > 100.f || _skillBox.Center.z < -200.f || _skillBox.Center.z > 100.f) {
			return true;
		}

		if (_projectile) {
			if (SkillMapCollision(_skillBox))
				return true;
		}

		int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)[3];
		if (bossID != -1) {
			if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(_skillBox)) {
				CObjectMgr::GetInstance()->GetClient(bossID)->Damage(_power, _critical, _type, _clientID);
				return true;
			}
		}

		for (int i = 0; i < MAX_MINION; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_matchNum, i);
			if (!npc->m_active)
				continue;
			if (npc->GetBoundingBox().Intersects(_skillBox)) {
				npc->Damaged(_clientID, _power, _type);
				return true;
			}
		}

		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_matchNum, i);
			if (!npc->m_active)
				continue;
			if (npc->GetBoundingBox().Intersects(_skillBox)) {
				npc->Damaged(_clientID, _power, _type);
				return true;
			}
		}

		return false;
	}

	bool GameUtil::BossSkillCollisionCheck(const DirectX::BoundingOrientedBox _skillBox, int _matchNum, int _power, int _critical, DAMAGE_TYPE _type, int _clientID, bool _projectile)
	{
		if (_skillBox.Center.x < -200.f || _skillBox.Center.x > 100.f || _skillBox.Center.z < -200.f || _skillBox.Center.z > 100.f) {
			return true;
		}

		if (_projectile) {
			if (SkillMapCollision(_skillBox))
				return true;
		}

		//Hero Collide Check
		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum);
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (client->GetBoundingBox().Intersects(_skillBox)) {
				client->Damage(_power, _critical, _type, _clientID);
				return true;
			}
		}

		//Monster Collide Check
		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_matchNum, i);
			if (!npc->m_active)
				continue;
			if (npc->GetBoundingBox().Intersects(_skillBox)) {
				npc->Damaged(_clientID, _power, _type);
				return true;
			}
		}

		return false;
	}

	bool GameUtil::SkillMapCollision(const DirectX::BoundingOrientedBox _skillBox)
	{
		if (_skillBox.Center.x < -200.f || _skillBox.Center.x > 100.f || _skillBox.Center.z < -200.f || _skillBox.Center.z > 100.f) {
			return true;
		}

		int sectionX = static_cast<int>((_skillBox.Center.x + 200) / (WORLD_WIDTH / SECTION_NUM));
		int sectionZ = static_cast<int>((_skillBox.Center.z + 200) / (WORLD_HEIGHT / SECTION_NUM));
		int checkSide = 0;
		int checkTopBottom = 0;

		if (sectionX == 0) {
			checkSide = 1;	//don't check left
		}
		else if (sectionX == SECTION_NUM - 1) {
			checkSide = 2;	//don't check right
		}

		if (sectionZ == 0) {
			checkTopBottom = 1; //don't check top
		}
		else if (sectionZ == SECTION_NUM - 1) {
			checkTopBottom = 2; //don't check bottom
		}

		try {
			// Map:Client collsion check
			if (m_boundingBoxs[sectionX][sectionZ].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX][sectionZ]) {
					if (box.Intersects(_skillBox)) {
						return true;
					}
				}
			}
			if (checkSide != 1 && m_boundingBoxs[sectionX - 1][sectionZ].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX - 1][sectionZ]) {
					if (box.Intersects(_skillBox)) {
						return true;
					}
				}
			}
			if (checkSide != 2 && m_boundingBoxs[sectionX + 1][sectionZ].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX + 1][sectionZ]) {
					if (box.Intersects(_skillBox)) {
						return true;
					}
				}
			}
			if (checkTopBottom != 1 && m_boundingBoxs[sectionX][sectionZ - 1].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX][sectionZ - 1]) {
					if (box.Intersects(_skillBox)) {
						return true;
					}
				}
			}
			if (checkTopBottom != 2 && m_boundingBoxs[sectionX][sectionZ + 1].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX][sectionZ + 1]) {
					if (box.Intersects(_skillBox)) {
						return true;
					}
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(GameUtil SkillCollisionCheck), " + std::string(ex.what()));
			return false;
		}

		return false;
	}

	DirectX::BoundingOrientedBox GameUtil::GenerateShortRangeBox(const vec3& _pos, const vec3& _look, const vec3& _extent, const vec3& _scale, const DirectX::XMFLOAT4X4& _world)
	{
		DirectX::XMFLOAT4X4 worldMatrix;
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());

		vec3 offset = _look; offset.m_y = 0.908f;
		vec3 newPos = _pos + offset;

		vec3 scaleValue = { _extent.m_x * 2.f * _scale.m_x, _extent.m_y * 2.f, _extent.m_z * 2.f * _scale.m_z };
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&worldMatrix), DirectX::XMMatrixScaling(scaleValue.m_x, scaleValue.m_y, scaleValue.m_z)));
		worldMatrix._41 = newPos.m_x; worldMatrix._42 = newPos.m_y; worldMatrix._43 = newPos.m_z;

		DirectX::XMVECTOR scaleV, rotationV, translationV;
		DirectX::XMMatrixDecompose(&scaleV, &rotationV, &translationV, XMLoadFloat4x4(&worldMatrix));

		DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&_world);

		playerWorldMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		DirectX::BoundingOrientedBox box;
		DirectX::XMVECTOR posVec = { newPos.m_x, newPos.m_y, newPos.m_z };
		box.Transform(box, (DirectX::XMMatrixScalingFromVector(scaleV) * playerWorldMatrix * DirectX::XMMatrixTranslationFromVector(posVec)));

		return box;
	}

	bool GameUtil::CheckJumpCollision(const vec3& _pos, int& _jumpNum)
	{
		for (int i = 0; i < JUMP_START_POS.size(); ++i) {
			if (::sqrt(::pow(JUMP_START_POS[i].m_x - _pos.m_x, 2) + ::pow(JUMP_START_POS[i].m_z - _pos.m_z, 2)) < 2.f) {
				_jumpNum = i;
				return true;
			}
		}
		_jumpNum = -1;
		return false;
	}

	const vec3 GameUtil::GetJumpPos(int _jumpNum, float _time)
	{
		float otherTime = 1.0f - _time;
		float powT = _time * _time;
		float powOtherTime = otherTime * otherTime;

		vec3 p1 = { JUMP_START_POS[_jumpNum].m_x, 2.f, JUMP_START_POS[_jumpNum].m_z };
		vec3 p2 = JUMP_CONTROL_POS[_jumpNum];
		vec3 p3 = { JUMP_LANDING_POS[_jumpNum].m_x, 2.f, JUMP_LANDING_POS[_jumpNum].m_z };


		return vec3(powOtherTime * p1.m_x + 2 * otherTime * _time * p2.m_x + powT * p3.m_x,
					powOtherTime * p1.m_y + 2 * otherTime * _time * p2.m_y + powT * p3.m_y,
					powOtherTime * p1.m_z + 2 * otherTime * _time * p2.m_z + powT * p3.m_z);
	}

	bool GameUtil::CheckTeleportCollision(const vec3& _pos, int& _teleport)
	{
		for (int i = 0; i < TELEPORT_POS.size(); ++i) {
			if (::sqrt(::pow(TELEPORT_POS[i].m_x - _pos.m_x, 2) + ::pow(TELEPORT_POS[i].m_z - _pos.m_z, 2)) < TELEPORT_INTERACTION_DISTANCE) {
				_teleport = i;
				return true;
			}
		}
		return false;
	}

	bool GameUtil::GetSlidingVector(const vec3& _pos, const vec3& _shift, int _nodeNum, vec3& _slidingVector)
	{
		if (_nodeNum < 0 || static_cast<size_t>(_nodeNum) >= m_naviMesh.size()) return false;
		const Triangle& triangle = m_naviMesh[_nodeNum]->m_triangle;

		int edgeIndex = FindEdgeAdjacentToPosition(_pos, triangle);
		if (edgeIndex != -1) {
			vec3 edge = triangle.GetEdge(edgeIndex);
			vec3 edgeNormal = vec3::Normalize(vec3(-edge.m_z, 0.0f, edge.m_x));

			vec3 directionXZ = vec3(_shift.m_x, 0.0f, _shift.m_z);
			_slidingVector = directionXZ - edgeNormal * edgeNormal.Dot(directionXZ);

			_slidingVector = vec3::Normalize(_slidingVector);
			return true;
		}
		return false;
	}

	int GameUtil::FindEdgeAdjacentToPosition(const vec3& _pos, const Triangle& _triangle)
	{
		int closestEdgeIndex = -1;
		float closestDistanceSquared = FLT_MAX;

		for (int i = 0; i < 3; ++i) {
			vec3 v1 = _triangle.GetVertex(i);
			vec3 v2 = _triangle.GetVertex((i + 1) % 3);

			float distanceSquared = DistanceToEdgeSquared(_pos, v1, v2);
			if (distanceSquared < closestDistanceSquared) {
				closestDistanceSquared = distanceSquared;
				closestEdgeIndex = i;
			}
		}

		return closestEdgeIndex;
	}

	float GameUtil::DistanceToEdgeSquared(const vec3& _point, const vec3& _edgeStart, const vec3& _edgeEnd)
	{
		vec3 edgeDir = _edgeEnd - _edgeStart;
		vec3 pointToEdge = _point - _edgeStart;

		float edgeLengthSquared = edgeDir.Length();
		float dotProduct = pointToEdge.Dot(edgeDir);

		float t = dotProduct / edgeLengthSquared;
		vec3 closestPoint;
		if (t < 0.0f)
			closestPoint = _edgeStart;
		else if (t > 1.0f)
			closestPoint = _edgeEnd;
		else
			closestPoint = _edgeStart + edgeDir * t;

		float distanceSquared = (_point - closestPoint).Length();
		return distanceSquared;
	}

	bool GameUtil::CheckTriangleAdjacent(const Triangle& _triangle1, const Triangle& _triangle2)
	{
		int sharedVertices = 0;

		if (_triangle2.ContainsEdge(_triangle1.m_v1, _triangle1.m_v2) || _triangle2.ContainsEdge(_triangle1.m_v2, _triangle1.m_v3) || _triangle2.ContainsEdge(_triangle1.m_v3, _triangle1.m_v1))
			sharedVertices += 2;

		if (_triangle1.ContainsEdge(_triangle2.m_v1, _triangle2.m_v2) || _triangle1.ContainsEdge(_triangle2.m_v2, _triangle2.m_v3) || _triangle1.ContainsEdge(_triangle2.m_v3, _triangle2.m_v1))
			sharedVertices += 2;

		return sharedVertices == 2;
	}
}