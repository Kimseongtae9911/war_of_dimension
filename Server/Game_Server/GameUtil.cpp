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
	std::array<std::vector<vec2>, PATH_NUM> GameUtil::minionPaths = { {} };
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

	bool GameUtil::LoadCoolTime(std::string filename)
	{
		std::fstream in(filename);
		
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

	bool GameUtil::LoadManaConsumption(std::string filename)
	{
		std::fstream in(filename);

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

	bool GameUtil::LoadNaviMesh(std::string filename)
	{
		std::ifstream in(filename);
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
				ss >> vertex.x >> vertex.y >> vertex.z;
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
					if (&node->triangle != &currentNodePtr->triangle && CheckTriangleAdjacent(node->triangle, currentNodePtr->triangle))
					{
						currentNodePtr->adjacentNodes.push_back(node);
						node->adjacentNodes.push_back(currentNodePtr);
					}
				}
			}
			
		}

		return true;
	}

	bool GameUtil::LoadHeightMesh(std::string filename)
	{
		std::ifstream in(filename);
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
				ss >> vertex.x >> vertex.y >> vertex.z;
				vertices.push_back(vertex);
			}
			else if (token == "f") {
				int index1, index2, index3;
				ss >> index1 >> index2 >> index3;
				m_heightMesh.push_back(Triangle(vertices[index1 - 1], vertices[index2 - 1], vertices[index3 - 1]));
			}
		}

		std::sort(m_heightMesh.begin(), m_heightMesh.end(), [](const Triangle& t1, const Triangle& t2) {
			return (t1.v1.y + t1.v2.y + t1.v3.y) / 3.f > (t2.v1.y + t2.v2.y + t2.v3.y) / 3.f;
			});

		return true;
	}

	bool GameUtil::LoadMap(std::string filename)
	{
		std::fstream in(filename);
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

	bool GameUtil::LoadPlayerBB(std::string filename, int type)
	{
		std::fstream in(filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		int objectNum = -1;
		in >> boxCnt;

		in >> objectNum;

		//0:HeroPlayer, 1:BossPlayer
		if (type == 0) {
			in >> m_playerInitBB.offset.x;
			in >> m_playerInitBB.offset.y;
			in >> m_playerInitBB.offset.z;

			in >> m_playerInitBB.extent.x;
			in >> m_playerInitBB.extent.y;
			in >> m_playerInitBB.extent.z;
		}
		else {
			in >> m_bossPlayerInitBB.offset.x;
			in >> m_bossPlayerInitBB.offset.y;
			in >> m_bossPlayerInitBB.offset.z;

			in >> m_bossPlayerInitBB.extent.x;
			in >> m_bossPlayerInitBB.extent.y;
			in >> m_bossPlayerInitBB.extent.z;
		}
		return true;
	}

	bool GameUtil::LoadMinionBB(std::string filename)
	{
		std::fstream in(filename);
		if (in.fail())
			return false;

		int boxCnt = 0;
		int objectNum = -1;
		in >> boxCnt;

		in >> objectNum;

		in >> m_minionInitBB.offset.x;
		in >> m_minionInitBB.offset.y;
		in >> m_minionInitBB.offset.z;

		in >> m_minionInitBB.extent.x;
		in >> m_minionInitBB.extent.y;
		in >> m_minionInitBB.extent.z;

		return true;
	}

	bool GameUtil::LoadFenceBB(std::string filename)
	{
		std::fstream in(filename);
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

	bool GameUtil::LoadMinionPath(std::string filename, int pathNum)
	{
		std::fstream in(filename);

		if (in.fail())
			return false;

		char c;
		float x, z;
		while (!in.eof()) {
			in >> c;
			in >> x >> z;

			minionPaths[pathNum].push_back({ x, z });
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

			in[i].first >> temp->offset.x;
			in[i].first >> temp->offset.y;
			in[i].first >> temp->offset.z;

			in[i].first >> temp->extent.x;
			in[i].first >> temp->extent.y;
			in[i].first >> temp->extent.z;

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

	bool GameUtil::LoadNexusBB(const std::string& filename)
	{
		std::fstream in(filename);
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

	int GameUtil::GetCoolTime(int characterNum, int skillNum)
	{
		if (m_playerSkillCoolTimes[characterNum].contains(skillNum))
			return m_playerSkillCoolTimes[characterNum][skillNum];
		else {
			LogPrinter::PrintMsg("Failed To Return CoolTime " + std::to_string(skillNum));
			return 0;
		}
	}

	int GameUtil::GetMpConsumption(int characterNum, int skillNum)
	{
		if (m_playerManaConsumption[characterNum].contains(skillNum))
			return m_playerManaConsumption[characterNum][skillNum];
		else {
			LogPrinter::PrintMsg("Failed To Return Mp Consumption " + std::to_string(skillNum));
			return 0;
		}
	}

	bool GameUtil::MapCollision(const vec3& pos, float& height, int& nodeNum)
	{
		Ray ray;
		ray.origin = pos;
		ray.origin.y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, height, nodeNum)) {
			ray.RayCast(m_heightMesh, height);
			return true;
		}

		return false;
	}

	bool GameUtil::MapCollision(const vec3& pos, vec3& shift, float& height, int& nodeNum)
	{
		Ray ray;
		ray.origin = pos + shift;
		ray.origin.y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, height, nodeNum)) {
			ray.RayCast(m_heightMesh, height);
			return true;
		}
		else {
			vec3 slidingVector;
			if (GetSlidingVector(pos, shift, nodeNum, slidingVector)) {
				shift = slidingVector * shift.Length();
				ray.origin = pos + shift;
				ray.origin.y += RAY_OFFSET;
				if (ray.RayCast(m_naviMesh, height, nodeNum)) {
					return false;
				}
				else {
					float desiredMagnitude = slidingVector.Length() * 2.0f;
					slidingVector = vec3::Normalize(slidingVector) * desiredMagnitude;
					vec3 temp = shift;
					shift = slidingVector * shift.Length() - temp;
					return false; 
				}
			}
			else {
				std::cout << "Failed To Get Sliding Vector" << std::endl;
			}
		}
		return false;
	}

	bool GameUtil::MapCollision(const vec3& pos, float& height, int& nodeNum, bool skill)
	{
		Ray ray;
		ray.origin = pos;
		ray.origin.y += RAY_OFFSET;

		if (ray.RayCast(m_naviMesh, height, nodeNum, skill)) {
			ray.RayCast(m_heightMesh, height, skill);
			return true;
		}
		return false;
	}

	bool GameUtil::FenceCollision(const DirectX::BoundingOrientedBox box, const vec3& shift)
	{
		DirectX::BoundingOrientedBox clientOBB = box;
		clientOBB.Center.x += shift.x; clientOBB.Center.y += shift.y; clientOBB.Center.z += shift.z;

		for (const auto& fence : m_magneticFences) {
			if (true == fence.Intersects(clientOBB))
				return true;
		}
		return false;
	}

	bool GameUtil::ClientCollisionCheck(int id, const vec3& shift)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);		

		DirectX::BoundingOrientedBox clientOBB = client->GetBoundingBox();
		clientOBB.Center.x += shift.x; clientOBB.Center.y += shift.y; clientOBB.Center.z += shift.z;

		for (int clID : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
			if (clID == -1 || clID == id)
				continue;
			
			if (clientOBB.Intersects(CObjectMgr::GetInstance()->GetClient(clID)->GetBoundingBox())) {
				return false;
			}
		}

		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);

			if (!npc->active)
				continue;

			if (clientOBB.Intersects(npc->GetBoundingBox()))
				return false;
		}
		

		return true;
	}

	bool GameUtil::NpcCollisionCheck(int id, int matchNum, const vec3& shift)
	{
		std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, id - NPC_ID);

		DirectX::BoundingOrientedBox npcOBB = npc->GetBoundingBox();
		npcOBB.Center.x += shift.x; npcOBB.Center.y += shift.y; npcOBB.Center.z += shift.z;

		for (int clID : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
			if (clID == -1)
				continue;

			if (npcOBB.Intersects(CObjectMgr::GetInstance()->GetClient(clID)->GetBoundingBox())) {
				return false;
			}
		}

		for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> otherNpc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);

			if (!otherNpc->active)
				continue;

			if (otherNpc->GetID() == id)
				continue;

			if (npcOBB.Intersects(otherNpc->GetBoundingBox())) {
				//Calculate and Apply Sliding Vector
				vec3 npcPos = npc->GetPos();
				vec3 newPosition = npcPos + vec3::Reflect(shift * 2.0f, vec3::Normalize(npcPos - otherNpc->GetPos()));
				npc->SetPos(newPosition);
				return false;
			}
		}


		return true;
	}

	bool GameUtil::HeroSkillCollisionCheck(const DirectX::BoundingOrientedBox skillBox, int matchNum, int power, int critical, DAMAGE_TYPE type, int clientID, bool projectile)
	{
		if (skillBox.Center.x < -200.f || skillBox.Center.x > 100.f || skillBox.Center.z < -200.f || skillBox.Center.z > 100.f) {
			return true;
		}

		if (projectile) {
			if (SkillMapCollision(skillBox))
				return true;
		}

		int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)[3];
		if (bossID != -1) {
			if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(skillBox)) {
				CObjectMgr::GetInstance()->GetClient(bossID)->Damage(power, critical, type, clientID);
				return true;
			}
		}

		for (int i = 0; i < MAX_MINION; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
			if (!npc->active)
				continue;
			if (npc->GetBoundingBox().Intersects(skillBox)) {
				npc->Damaged(clientID, power, type);
				return true;
			}
		}

		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
			if (!npc->active)
				continue;
			if (npc->GetBoundingBox().Intersects(skillBox)) {
				npc->Damaged(clientID, power, type);
				return true;
			}
		}

		return false;
	}

	bool GameUtil::BossSkillCollisionCheck(const DirectX::BoundingOrientedBox skillBox, int matchNum, int power, int critical, DAMAGE_TYPE type, int clientID, bool projectile)
	{
		if (skillBox.Center.x < -200.f || skillBox.Center.x > 100.f || skillBox.Center.z < -200.f || skillBox.Center.z > 100.f) {
			return true;
		}

		if (projectile) {
			if (SkillMapCollision(skillBox))
				return true;
		}

		//Hero Collide Check
		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum);
		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (client->GetBoundingBox().Intersects(skillBox)) {
				client->Damage(power, critical, type, clientID);
				return true;
			}
		}

		//Monster Collide Check
		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
			if (!npc->active)
				continue;
			if (npc->GetBoundingBox().Intersects(skillBox)) {
				npc->Damaged(clientID, power, type);
				return true;
			}
		}

		return false;
	}

	bool GameUtil::SkillMapCollision(const DirectX::BoundingOrientedBox skillBox)
	{
		if (skillBox.Center.x < -200.f || skillBox.Center.x > 100.f || skillBox.Center.z < -200.f || skillBox.Center.z > 100.f) {
			return true;
		}

		int sectionX = static_cast<int>((skillBox.Center.x + 200) / (WORLD_WIDTH / SECTION_NUM));
		int sectionZ = static_cast<int>((skillBox.Center.z + 200) / (WORLD_HEIGHT / SECTION_NUM));
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
					if (box.Intersects(skillBox)) {
						return true;
					}
				}
			}
			if (checkSide != 1 && m_boundingBoxs[sectionX - 1][sectionZ].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX - 1][sectionZ]) {
					if (box.Intersects(skillBox)) {
						return true;
					}
				}
			}
			if (checkSide != 2 && m_boundingBoxs[sectionX + 1][sectionZ].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX + 1][sectionZ]) {
					if (box.Intersects(skillBox)) {
						return true;
					}
				}
			}
			if (checkTopBottom != 1 && m_boundingBoxs[sectionX][sectionZ - 1].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX][sectionZ - 1]) {
					if (box.Intersects(skillBox)) {
						return true;
					}
				}
			}
			if (checkTopBottom != 2 && m_boundingBoxs[sectionX][sectionZ + 1].empty() == false) {
				for (auto& box : m_boundingBoxs[sectionX][sectionZ + 1]) {
					if (box.Intersects(skillBox)) {
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

	DirectX::BoundingOrientedBox GameUtil::GenerateShortRangeBox(const vec3& pos, const vec3& look, const vec3& extent, const vec3& scale, const DirectX::XMFLOAT4X4& world)
	{
		DirectX::XMFLOAT4X4 worldMatrix;
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixIdentity());

		vec3 offset = look; offset.y = 0.908f;
		vec3 newPos = pos + offset;

		vec3 scaleValue = { extent.x * 2.f * scale.x, extent.y * 2.f, extent.z * 2.f * scale.z };
		DirectX::XMStoreFloat4x4(&worldMatrix, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(&worldMatrix), DirectX::XMMatrixScaling(scaleValue.x, scaleValue.y, scaleValue.z)));
		worldMatrix._41 = newPos.x; worldMatrix._42 = newPos.y; worldMatrix._43 = newPos.z;

		DirectX::XMVECTOR scaleV, rotationV, translationV;
		DirectX::XMMatrixDecompose(&scaleV, &rotationV, &translationV, XMLoadFloat4x4(&worldMatrix));

		DirectX::XMMATRIX playerWorldMatrix = DirectX::XMLoadFloat4x4(&world);

		playerWorldMatrix.r[3] = DirectX::XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);

		DirectX::BoundingOrientedBox box;
		DirectX::XMVECTOR posVec = { newPos.x, newPos.y, newPos.z };
		box.Transform(box, (DirectX::XMMatrixScalingFromVector(scaleV) * playerWorldMatrix * DirectX::XMMatrixTranslationFromVector(posVec)));

		return box;
	}

	bool GameUtil::CheckJumpCollision(const vec3& pos, int& jumpNum)
	{
		for (int i = 0; i < JUMP_START_POS.size(); ++i) {
			if (::sqrt(::pow(JUMP_START_POS[i].x - pos.x, 2) + ::pow(JUMP_START_POS[i].z - pos.z, 2)) < 2.f) {
				jumpNum = i;
				return true;
			}
		}
		jumpNum = -1;
		return false;
	}

	const vec3 GameUtil::GetJumpPos(int jumpNum, float time)
	{
		float otherTime = 1.0f - time;
		float powT = time * time;
		float powOtherTime = otherTime * otherTime;

		vec3 p1 = { JUMP_START_POS[jumpNum].x, 2.f, JUMP_START_POS[jumpNum].z };
		vec3 p2 = JUMP_CONTROL_POS[jumpNum];
		vec3 p3 = { JUMP_LANDING_POS[jumpNum].x, 2.f, JUMP_LANDING_POS[jumpNum].z };


		return vec3(powOtherTime * p1.x + 2 * otherTime * time * p2.x + powT * p3.x,
					powOtherTime * p1.y + 2 * otherTime * time * p2.y + powT * p3.y,
					powOtherTime * p1.z + 2 * otherTime * time * p2.z + powT * p3.z);
	}

	bool GameUtil::CheckTeleportCollision(const vec3& pos, int& teleport)
	{
		for (int i = 0; i < TELEPORT_POS.size(); ++i) {
			if (::sqrt(::pow(TELEPORT_POS[i].x - pos.x, 2) + ::pow(TELEPORT_POS[i].z - pos.z, 2)) < TELEPORT_INTERACTION_DISTANCE) {
				teleport = i;
				return true;
			}
		}
		return false;
	}

	bool GameUtil::GetSlidingVector(const vec3& pos, const vec3& shift, int nodeNum, vec3& slidingVector)
	{
		if (nodeNum < 0 || static_cast<size_t>(nodeNum) >= m_naviMesh.size()) return false;
		const Triangle& triangle = m_naviMesh[nodeNum]->triangle;

		int edgeIndex = FindEdgeAdjacentToPosition(pos, triangle);
		if (edgeIndex != -1) {
			vec3 edge = triangle.GetEdge(edgeIndex);
			vec3 edgeNormal = vec3::Normalize(vec3(-edge.z, 0.0f, edge.x));

			vec3 directionXZ = vec3(shift.x, 0.0f, shift.z);
			slidingVector = directionXZ - edgeNormal * edgeNormal.Dot(directionXZ);
	
			slidingVector = vec3::Normalize(slidingVector);
			return true;
		}
		return false;
	}

	int GameUtil::FindEdgeAdjacentToPosition(const vec3& pos, const Triangle& triangle)
	{
		int closestEdgeIndex = -1;
		float closestDistanceSquared = FLT_MAX;

		for (int i = 0; i < 3; ++i) {
			vec3 v1 = triangle.GetVertex(i);
			vec3 v2 = triangle.GetVertex((i + 1) % 3);

			float distanceSquared = DistanceToEdgeSquared(pos, v1, v2);
			if (distanceSquared < closestDistanceSquared) {
				closestDistanceSquared = distanceSquared;
				closestEdgeIndex = i;
			}
		}

		return closestEdgeIndex;
	}

	float GameUtil::DistanceToEdgeSquared(const vec3& point, const vec3& edgeStart, const vec3& edgeEnd)
	{
		vec3 edgeDir = edgeEnd - edgeStart;
		vec3 pointToEdge = point - edgeStart;

		float edgeLengthSquared = edgeDir.Length();
		float dotProduct = pointToEdge.Dot(edgeDir);

		float t = dotProduct / edgeLengthSquared;
		vec3 closestPoint;
		if (t < 0.0f)
			closestPoint = edgeStart;
		else if (t > 1.0f)
			closestPoint = edgeEnd;
		else
			closestPoint = edgeStart + edgeDir * t;

		float distanceSquared = (point - closestPoint).Length();
		return distanceSquared;
	}

	bool GameUtil::CheckTriangleAdjacent(const Triangle& triangle1, const Triangle& triangle2)
	{
		int sharedVertices = 0;

		if (triangle2.ContainsEdge(triangle1.v1, triangle1.v2) || triangle2.ContainsEdge(triangle1.v2, triangle1.v3) || triangle2.ContainsEdge(triangle1.v3, triangle1.v1))
			sharedVertices += 2;

		if (triangle1.ContainsEdge(triangle2.v1, triangle2.v2) || triangle1.ContainsEdge(triangle2.v2, triangle2.v3) || triangle1.ContainsEdge(triangle2.v3, triangle2.v1))
			sharedVertices += 2;
		
		return sharedVertices == 2;
	}
}