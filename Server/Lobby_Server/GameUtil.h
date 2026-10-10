#pragma once

#define SECTION_NUM 10
constexpr int WORLD_WIDTH = 600;
constexpr int WORLD_HEIGHT = 600;

namespace wod_server {

	struct PlayerBoundingBox {
		vec3 m_offset;
		vec3 m_extent;
	};

	struct Section {
		std::vector<DirectX::BoundingOrientedBox> m_boundingBoxs;
		std::vector<int> m_npcs;
		std::unordered_set<int> m_clients;

		std::shared_mutex m_clientLock;
	};

	class GameUtil
	{
	public:
		static bool LoadNaviMesh(std::string _filename);
		static bool LoadHeightMesh(std::string _filename);
		static bool LoadMap(std::string _filename);

		static bool MapCollision(const vec3& _rayPos, float& _height);

		static void RegisterClientToSection(int _x, int _z, int _s) { m_sections[_x][_z].m_clientLock.lock(); m_sections[_x][_z].m_clients.insert(_s); m_sections[_x][_z].m_clientLock.unlock(); }
		static void UpdateSection(int _socketNum, int _beforeX, int _beforeZ, int _sectionX, int _sectionZ);
		static void RemoveClientFromSection(int _x, int _z, int _s) { m_sections[_x][_z].m_clientLock.lock(); m_sections[_x][_z].m_clients.erase(_s); m_sections[_x][_z].m_clientLock.unlock(); }

	private:
		static std::vector<Triangle> m_naviMesh;
		static std::vector<Triangle> m_heightMesh;
		static PlayerBoundingBox m_playerInitBB;
		static std::array<std::array<Section, SECTION_NUM>, SECTION_NUM> m_sections;
	};
}