#include "pch.h"
#include "CUserMgr.h"

namespace wod_server {
	std::vector<Triangle> GameUtil::m_naviMesh;
	std::vector<Triangle> GameUtil::m_heightMesh;
    TerrainSpatialIndex GameUtil::m_naviIndex;
    TerrainSpatialIndex GameUtil::m_heightIndex;
	PlayerBoundingBox GameUtil::m_playerInitBB;
	std::array<std::array<Section, SECTION_NUM>, SECTION_NUM> GameUtil::m_sections;

	bool GameUtil::LoadNaviMesh(std::string _filename)
	{
		std::ifstream in(_filename);
		if (in.fail())
			return false;

        m_naviIndex.Reset();
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
				m_naviMesh.push_back(Triangle(vertices[index1 - 1], vertices[index2 - 1], vertices[index3 - 1]));
			}
		}

        const bool indexed = m_naviIndex.Build(m_naviMesh.size(), [](size_t _id) -> const Triangle& {
            return m_naviMesh[_id];
        });
        LogPrinter::PrintMsg("Lobby NavMesh spatial index=" + std::to_string(indexed) + " cells=" + std::to_string(m_naviIndex.Cells()) + " global=" + std::to_string(m_naviIndex.GlobalCandidates()));

		return true;
	}

	bool GameUtil::LoadHeightMesh(std::string _filename)
	{
		std::ifstream in(_filename);
		if (in.fail())
			return false;

        m_heightIndex.Reset();
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

        const bool indexed = m_heightIndex.Build(m_heightMesh.size(), [](size_t _id) -> const Triangle& {
            return m_heightMesh[_id];
        });
        LogPrinter::PrintMsg("Lobby HeightMesh spatial index=" + std::to_string(indexed) + " cells=" + std::to_string(m_heightIndex.Cells()) + " global=" + std::to_string(m_heightIndex.GlobalCandidates()));

		return true;
	}

    std::string GameUtil::TerrainIndexJson()
    {
        const auto describe = [](const TerrainSpatialIndex& _index, size_t _triangles) {
            return "{\"ready\":" + std::string(_index.Ready() ? "true" : "false") + ",\"triangles\":" + std::to_string(_triangles) + ",\"cells\":" + std::to_string(_index.Cells()) + ",\"references\":" +
                   std::to_string(_index.References()) + ",\"global\":" + std::to_string(_index.GlobalCandidates()) + ",\"bytes\":" + std::to_string(_index.Bytes()) + ",\"prepared_bytes\":" + std::to_string(_index.PreparedBytes()) + "}";
        };

        return "{\"schema_version\":1,\"nav\":" + describe(m_naviIndex, m_naviMesh.size()) + ",\"height\":" + describe(m_heightIndex, m_heightMesh.size()) + "}";
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
			obb.CreateFromBoundingBox(obb, DirectX::BoundingBox(DirectX::XMFLOAT3(center), DirectX::XMFLOAT3(0.5, 0.5, 0.5)));
			DirectX::XMVECTOR scaling = DirectX::XMVectorSet(scale[0], scale[1], scale[2], 0.f);
			DirectX::XMVECTOR quater = DirectX::XMVectorSet(quaternion[0], quaternion[1], quaternion[2], quaternion[3]);
			DirectX::XMMATRIX transformMatrix = DirectX::XMMatrixScalingFromVector(scaling) * DirectX::XMMatrixRotationQuaternion(quater);
			obb.Transform(obb, transformMatrix);
			obb.Center = { center[0], center[1], center[2] };
			m_sections[static_cast<int>((center[0] + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM))][static_cast<int>((center[2] + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM))].m_boundingBoxs.push_back(obb);
		}

		return true;
	}

    bool GameUtil::MapCollision(const vec3& _rayPos, float& _height)
    {
        wod::core::TickProfile::Scope collision(wod::core::TickProfile::Stage::MapCollision);
        Ray ray;
        ray.m_origin = _rayPos;
        ray.m_origin.m_y += RAY_OFFSET;
        bool navigable;
        {
            wod::core::TickProfile::Scope nav(wod::core::TickProfile::Stage::LobbyNav);
            navigable = ray.RayCast(m_naviMesh, _height, &m_naviIndex);
        }

        if (navigable)
        {
            wod::core::TickProfile::Scope height(wod::core::TickProfile::Stage::LobbyHeight);
            ray.RayCast(m_heightMesh, _height, &m_heightIndex);

            return true;
        }

        return false;
    }

	void GameUtil::UpdateSection(int _socketNum, int _beforeX, int _beforeZ, int _sectionX, int _sectionZ)
	{
		try {
			m_sections[_beforeX][_beforeZ].m_clientLock.lock();
			m_sections[_beforeX][_beforeZ].m_clients.erase(_socketNum);
			m_sections[_beforeX][_beforeZ].m_clientLock.unlock();

			m_sections[_sectionX][_sectionZ].m_clientLock.lock();
			m_sections[_sectionX][_sectionZ].m_clients.insert(_socketNum);
			m_sections[_sectionX][_sectionZ].m_clientLock.unlock();
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg("Err(GameUtil UpdateSection), " + std::string(ex.what()));
		}
	}

}
