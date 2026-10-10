#pragma once

namespace wod_server {
	struct InitBoundingBox {
		vec3 m_offset;
		vec3 m_extent;
	};

	class GameUtil
	{
	public:
		~GameUtil();

		static bool LoadCoolTime(std::string _filename);
		static bool LoadManaConsumption(std::string _filename);
		static bool LoadNaviMesh(std::string _filename);
		static bool LoadHeightMesh(std::string _filename);
		static bool LoadMap(std::string _filename);
		static bool LoadPlayerBB(std::string _filename, int _type);
		static bool LoadMinionBB(std::string _filename);
		static bool LoadFenceBB(std::string _filename);
		static bool LoadMinionPath(std::string _filename, int _pathNum);
		static bool LoadMonsterBB();
		static bool LoadTowerBB();
		static bool LoadNexusBB(const std::string& _filename);

		static int GetCoolTime(int _characterNum, int _skillNum);
		static int GetMpConsumption(int _characterNum, int _skillNum);

		static const DirectX::BoundingOrientedBox& GetNexusBB() { return m_nexusBB; }
		static const DirectX::BoundingOrientedBox& GetTowerBB(int _index) { return m_towerBBs[_index]; }

		static bool MapCollision(const vec3& _pos, float& _height, int& _nodeNum);
		static bool MapCollision(const vec3& _pos, vec3& _shift, float& _height, int& _nodeNum);
		static bool MapCollision(const vec3& _pos, float& _height, int& _nodeNum, bool _skill);
		static bool FenceCollision(const DirectX::BoundingOrientedBox _box, const vec3& _shift);
		static bool ClientCollisionCheck(int _id, const vec3& _shift);
		static bool NpcCollisionCheck(int _id, int _matchNum, const vec3& _shift);
		static void UpdateSection(int _id, int _matchNum, int _beforeX, int _beforeZ, int _sectionX, int _sectionZ);

		static bool HeroSkillCollisionCheck(const DirectX::BoundingOrientedBox _box, int _matchNum, int _power, int _critical, DAMAGE_TYPE _type, int _clientID, bool _projectile = true);
		static bool BossSkillCollisionCheck(const DirectX::BoundingOrientedBox _box, int _matchNum, int _power, int _critical, DAMAGE_TYPE _type, int _clientID, bool _projectile = true);
		static bool SkillMapCollision(const DirectX::BoundingOrientedBox _skillBox);	//Skill - Map Collision Check

		static DirectX::BoundingOrientedBox GenerateShortRangeBox(const vec3& _pos, const vec3& _look, const vec3& _extent, const vec3& _scale, const DirectX::XMFLOAT4X4& _world);

		static bool CheckJumpCollision(const vec3& _pos, int& _jumpNum);
		static const vec3 GetJumpPos(int _jumpNum, float _time);

		static bool CheckTeleportCollision(const vec3& _pos, int& _teleport);

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

		static Node* GetNode(int _index) { return m_naviMesh[_index]; }

		static std::array<std::vector<Vector2>, PATH_NUM> m_minionPaths;

		static bool GetSlidingVector(const vec3& _pos, const vec3& _shift, int _nodeNum, vec3& _slidingVector);
		static int FindEdgeAdjacentToPosition(const vec3& _pos, const Triangle& _triangle);
		static float DistanceToEdgeSquared(const vec3& _point, const vec3& _edgeStart, const vec3& _edgeEnd);

	private:
		static bool CheckTriangleAdjacent(const Triangle& _triangle1, const Triangle& _triangle2);

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