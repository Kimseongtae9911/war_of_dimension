#include "pch.h"
#include "CUserMgr.h"

namespace wod_server {	
	std::vector<Triangle> GameUtil::m_naviMesh;
	std::vector<Triangle> GameUtil::m_heightMesh;
	PlayerBoundingBox GameUtil::m_playerInitBB;
	std::array<std::array<Section, SECTION_NUM>, SECTION_NUM> GameUtil::m_sections;

	bool GameUtil::LoadNaviMesh(std::string filename)
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
				m_naviMesh.push_back(Triangle(vertices[index1 - 1], vertices[index2 - 1], vertices[index3 - 1]));
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
			obb.CreateFromBoundingBox(obb, DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.5, 0.5, 0.5)));
			DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
			DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
			obb.Transform(obb, transformMatrix);
			obb.Center = { center[0], center[1], center[2] };
			m_sections[static_cast<int>((center[0] + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM))][static_cast<int>((center[2] + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM))].boundingBoxs.push_back(obb);
		}

		return true;
	}

	bool GameUtil::MapCollision(const vec3& rayPos, float& height)
	{
		Ray ray;
		ray.origin = rayPos;
		ray.origin.y += RAY_OFFSET;
		if (true == ray.RayCast(m_naviMesh, height)) {
			ray.RayCast(m_heightMesh, height);
			return true;
		}
		return false;
	}

	void GameUtil::UpdateSection(int socketNum, int beforeX, int beforeZ, int sectionX, int sectionZ)
	{
		try {
			m_sections[beforeX][beforeZ].clientLock.lock();
			m_sections[beforeX][beforeZ].clients.erase(socketNum);
			m_sections[beforeX][beforeZ].clientLock.unlock();

			m_sections[sectionX][sectionZ].clientLock.lock();
			m_sections[sectionX][sectionZ].clients.insert(socketNum);
			m_sections[sectionX][sectionZ].clientLock.unlock();
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg("Err(GameUtil UpdateSection), " + std::string(ex.what()));
		}
	}

}