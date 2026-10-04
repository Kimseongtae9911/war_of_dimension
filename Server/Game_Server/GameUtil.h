#pragma once

namespace wod_server {
	struct InitBoundingBox {
		vec3 offset;
		vec3 extent;
	};

	class GameUtil
	{
	public:
		~GameUtil();

		static bool LoadCoolTime(std::string filename);
		static bool LoadManaConsumption(std::string filename);
		static bool LoadNaviMesh(std::string filename);
		static bool LoadHeightMesh(std::string filename);
		static bool LoadMap(std::string filename);
		static bool LoadPlayerBB(std::string filename, int type);
		static bool LoadMinionBB(std::string filename);
		static bool LoadFenceBB(std::string filename);
		static bool LoadMinionPath(std::string filename, int pathNum);
		static bool LoadMonsterBB();
		static bool LoadTowerBB();
		static bool LoadNexusBB(const std::string& filename);

		static int GetCoolTime(int characterNum, int skillNum);
		static int GetMpConsumption(int characterNum, int skillNum);
		
		static const DirectX::BoundingOrientedBox& GetNexusBB() { return m_nexusBB; }
		static const DirectX::BoundingOrientedBox& GetTowerBB(int index) { return m_towerBBs[index]; }

		static bool MapCollision(const vec3& pos, float& height, int& nodeNum);
		static bool MapCollision(const vec3& pos, vec3& shift, float& height, int& nodeNum);
		static bool MapCollision(const vec3& pos, float& height, int& nodeNum, bool skill);
		static bool FenceCollision(const DirectX::BoundingOrientedBox box, const vec3& shift);
		static bool ClientCollisionCheck(int id, const vec3& shift);
		static bool NpcCollisionCheck(int id, int matchNum, const vec3& shift);
		static void UpdateSection(int id, int matchNum, int beforeX, int beforeZ, int sectionX, int sectionZ);

		static bool HeroSkillCollisionCheck(const DirectX::BoundingOrientedBox box, int matchNum, int power, int critical, DAMAGE_TYPE type, int clientID, bool projectile = true);
		static bool BossSkillCollisionCheck(const DirectX::BoundingOrientedBox box, int matchNum, int power, int critical, DAMAGE_TYPE type, int clientID, bool projectile = true);
		static bool SkillMapCollision(const DirectX::BoundingOrientedBox skillBox);	//Skill - Map Collision Check

		static DirectX::BoundingOrientedBox GenerateShortRangeBox(const vec3& pos, const vec3& look, const vec3& extent, const vec3& scale, const DirectX::XMFLOAT4X4& world);

		static bool CheckJumpCollision(const vec3& pos, int& jumpNum);
		static const vec3 GetJumpPos(int jumpNum, float time);

		static bool CheckTeleportCollision(const vec3& pos, int& teleport);

		static const InitBoundingBox& GetPlayerInitBB() { return m_playerInitBB; }
		static const InitBoundingBox& GetBossPlayerInitBB() { return m_bossPlayerInitBB; }
		static const InitBoundingBox& GetMinionInitBB() { return m_minionInitBB; }
		static const InitBoundingBox* GetMonsterInitBB(ENpcType _type)
		{
			auto it = m_monsterInitBB.find(_type);
			if (it == m_monsterInitBB.end())
			{
				LogPrinter::PrintMsg("Not Found ENpcType");
				return nullptr;
			}
			return it->second;
		}

		static Node* GetNode(int index) { return m_naviMesh[index]; }

		static std::array<std::vector<Vector2>, PATH_NUM> minionPaths;

		static bool GetSlidingVector(const vec3& pos, const vec3& shift, int nodeNum, vec3& slidingVector);
		static int FindEdgeAdjacentToPosition(const vec3& pos, const Triangle& triangle);
		static float DistanceToEdgeSquared(const vec3& point, const vec3& edgeStart, const vec3& edgeEnd);

	private:
		static bool CheckTriangleAdjacent(const Triangle& triangle1, const Triangle& triangle2);

		static std::array<std::unordered_map<int, int>, MAX_ROLE> m_playerSkillCoolTimes;	//0: archer, fighter, swordman, wizard, ogre, programmer
		static std::array<std::unordered_map<int, int>, MAX_ROLE> m_playerManaConsumption;	//0: archer, fighter, swordman, wizard, ogre, programmer
		static std::vector<Node*> m_naviMesh;
		static std::vector<Triangle> m_heightMesh;
		static std::array<std::array<std::vector<DirectX::BoundingOrientedBox>, SECTION_NUM>, SECTION_NUM> m_boundingBoxs;
		static InitBoundingBox m_playerInitBB;
		static InitBoundingBox m_bossPlayerInitBB;
		static InitBoundingBox m_minionInitBB;
		static std::map<ENpcType, InitBoundingBox*> m_monsterInitBB;
		static std::vector<DirectX::BoundingOrientedBox> m_magneticFences;
		static DirectX::BoundingOrientedBox m_nexusBB;
		static std::array<DirectX::BoundingOrientedBox, PATH_NUM> m_towerBBs;
	};
}