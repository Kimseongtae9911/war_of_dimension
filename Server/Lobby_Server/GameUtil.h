#pragma once

#define SECTION_NUM 10
constexpr int WORLD_WIDTH = 600;
constexpr int WORLD_HEIGHT = 600;

namespace wod_server {

	struct PlayerBoundingBox {
		vec3 offset;
		vec3 extent;
	};

	struct Section {
		std::vector<DirectX::BoundingOrientedBox> boundingBoxs;
		std::vector<int> npcs;
		std::unordered_set<int> clients;

		std::shared_mutex clientLock;
	};

	class GameUtil
	{
	public:
		static bool LoadNaviMesh(std::string filename);
		static bool LoadHeightMesh(std::string filename);
		static bool LoadMap(std::string filename);

		static bool MapCollision(const vec3& rayPos, float& height);

		static void RegisterClientToSection(int x, int z, int s) { m_sections[x][z].clientLock.lock(); m_sections[x][z].clients.insert(s); m_sections[x][z].clientLock.unlock(); }
		static void UpdateSection(int socketNum, int beforeX, int beforeZ, int sectionX, int sectionZ);
		static void RemoveClientFromSection(int x, int z, int s) { m_sections[x][z].clientLock.lock(); m_sections[x][z].clients.erase(s); m_sections[x][z].clientLock.unlock(); }

	private:
		static std::vector<Triangle> m_naviMesh;
		static std::vector<Triangle> m_heightMesh;
		static PlayerBoundingBox m_playerInitBB;
		static std::array<std::array<Section, SECTION_NUM>, SECTION_NUM> m_sections;
	};
}