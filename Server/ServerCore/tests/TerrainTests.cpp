#include <DirectXMath.h>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cfloat>
#include <cmath>
#include <fstream>
#include <future>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include "../../Game_Server/MathUtil.h"
#include "TerrainReferenceRay.h"

namespace
{
using namespace wod_server;
std::atomic<int> m_assertions = 0;

void Require(bool _condition, const char *_message)
{
    m_assertions.fetch_add(1, std::memory_order_relaxed);
    if (!_condition)
        throw std::runtime_error(_message);
}

// Check all four overloads against the frozen pre-change implementation.
size_t Equivalent(const std::vector<Triangle>& _mesh, const std::vector<Node *>& _nodes, const TerrainSpatialIndex& _index, const Vector3& _origin, const Vector3& _direction = {0, -1, 0})
{
    Ray indexed;
    ReferenceRay reference;
    indexed.m_origin = reference.m_origin = _origin;
    indexed.m_direction = reference.m_direction = _direction;
    for (bool skill : {false, true})
    {
        float expected = -1234.5f, actual = expected;
        int expectedId = -4321, actualId = expectedId;
        const bool heightHit = skill ? reference.RayCast(_mesh, expected, true) : reference.RayCast(_mesh, expected);
        const bool indexedHeightHit = skill ? indexed.RayCast(_mesh, actual, true, &_index) : indexed.RayCast(_mesh, actual, &_index);
        Require(heightHit == indexedHeightHit && std::bit_cast<uint32_t>(actual) == std::bit_cast<uint32_t>(expected), "height first-hit/float result mismatch");
        expected = actual = -1234.5f;
        const bool navHit = skill ? reference.RayCast(_nodes, expected, expectedId, true) : reference.RayCast(_nodes, expected, expectedId);
        const bool indexedNavHit = skill ? indexed.RayCast(_nodes, actual, actualId, true, &_index) : indexed.RayCast(_nodes, actual, actualId, &_index);
        Require(navHit == indexedNavHit && actualId == expectedId && std::bit_cast<uint32_t>(actual) == std::bit_cast<uint32_t>(expected), "nav first-hit/id/float result mismatch");
    }

    return 4;
}

struct Mesh
{
    std::vector<Triangle> m_triangles;
    std::vector<std::unique_ptr<Node>> m_owned;
    std::vector<Node *> m_nodes;
    TerrainSpatialIndex m_index;

    void Build()
    {
        m_owned.clear();
        m_nodes.clear();
        for (const auto& triangle : m_triangles)
        {
            m_owned.push_back(std::make_unique<Node>(triangle));
            m_nodes.push_back(m_owned.back().get());
        }

        Require(m_index.Build(m_triangles.size(),
                              [&](size_t _id) -> const Triangle& {
                                  return m_triangles[_id];
                              }),
                "index build failed");
    }
};

Mesh Load(const char *_path, bool _sort)
{
    std::ifstream stream(_path);
    Require(stream.good(), "terrain asset missing");
    Mesh mesh;
    std::vector<Vector3> vertices;
    std::string line;
    while (std::getline(stream, line))
    {
        std::stringstream fields(line);
        std::string kind;
        fields >> kind;
        if (kind == "v")
        {
            Vector3 vertex;
            fields >> vertex.m_x >> vertex.m_y >> vertex.m_z;
            vertices.push_back(vertex);
        }
        else if (kind == "f")
        {
            size_t a, b, c;
            fields >> a >> b >> c;
            Require(fields.good() || fields.eof(), "invalid face");
            mesh.m_triangles.emplace_back(vertices.at(a - 1), vertices.at(b - 1), vertices.at(c - 1), static_cast<int>(mesh.m_triangles.size()));
        }
    }

    if (_sort)
        std::sort(mesh.m_triangles.begin(), mesh.m_triangles.end(), [](const Triangle& _a, const Triangle& _b) {
            return (_a.m_v1.m_y + _a.m_v2.m_y + _a.m_v3.m_y) / 3.f > (_b.m_v1.m_y + _b.m_v2.m_y + _b.m_v3.m_y) / 3.f;
        });

    mesh.Build();

    return mesh;
}
}

int RunTerrainTests()
{
    const int initialAssertions = m_assertions.load();
    Mesh mesh;
    // Lower face comes first: nearest-hit sorting would change the result.
    mesh.m_triangles = {{{-8, 3, -8}, {8, 3, -8}, {-8, 3, 8}, 71}, {{-8, 6, -8}, {8, 6, -8}, {-8, 6, 8}, 13}, {{-3, 0, -3}, {3, 0, -3}, {-3, 0, 3}, 5}, {{0, 2, 0}, {0, 4, 0}, {0, 6, 0}, 8}, {{30, 3, 30}, {30.00001f, 4, 30}, {30, 5, 30.00001f}, 9}};
    mesh.Build();
    for (float x : {-100.f, -8.00001f, -8.f, -7.99999f, -4.f, 0.f, 4.f, 8.f, 30.f, 100.f})
        for (float z : {-8.00001f, -8.f, -4.f, 0.f, 4.f, 8.f})
            for (float y : {1.f, 3.f, 5.f, 9.f, 2048.f})
                Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {x, y, z});
    for (Vector3 direction : {Vector3{1, -1, 0}, Vector3{0, 1, 0}, Vector3{0, 0, 0}, Vector3{0, -2, 0}})
        Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {0, 9, 0}, direction);
    for (float value : {std::numeric_limits<float>::quiet_NaN(), INFINITY, -INFINITY})
    {
        Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {value, 9, 0});
        Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {0, value, 0});
    }

    // Invalid, reset and source-size mismatch fall back without dropping candidates.
    mesh.m_index.Reset();
    Require(mesh.m_index.Query(0, 0, mesh.m_triangles.size()).Size() == mesh.m_triangles.size(), "reset fallback");
    Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {0, 9, 0});
    Require(!mesh.m_index.Build(
                mesh.m_triangles.size(),
                [&](size_t _id) -> const Triangle& {
                    return mesh.m_triangles[_id];
                },
                0),
            "invalid grid size accepted");
    Require(!mesh.m_index.Build(
                mesh.m_triangles.size(),
                [&](size_t _id) -> const Triangle& {
                    return mesh.m_triangles[_id];
                },
                0.000001f),
            "unbounded cell allocation accepted");
    mesh.Build();
    Require(mesh.m_index.Query(0, 0, mesh.m_triangles.size() + 1).Size() == mesh.m_triangles.size() + 1, "size mismatch fallback");
    const size_t oldCells = mesh.m_index.Cells();
    Require(mesh.m_index.Build(0,
                               [&](size_t _id) -> const Triangle& {
                                   return mesh.m_triangles[_id];
                               }) &&
                mesh.m_index.Query(0, 0, 0).Size() == 0,
            "empty rebuild");
    Require(oldCells > mesh.m_index.Cells(), "old cells retained on rebuild");
    mesh.m_triangles[0].m_v1.m_x = std::numeric_limits<float>::quiet_NaN();
    Require(!mesh.m_index.Build(mesh.m_triangles.size(),
                                [&](size_t _id) -> const Triangle& {
                                    return mesh.m_triangles[_id];
                                }) &&
                !mesh.m_index.Ready(),
            "nonfinite build must disable index");

    return m_assertions.load() - initialAssertions;
}

int RunTerrainAssetChecks()
{
    size_t total = 0;
    std::cout << "{\"meshes\":[";
    bool comma = false;
    for (bool height : {false, true})
    {
        auto mesh = Load(height ? "Server/Game_Server/Resource/HeightMesh.obj" : "Server/Game_Server/Resource/NavMeshData3.obj", height);
        std::mt19937 generator(20261012);
        std::uniform_real_distribution<float> position(-220, 220);
        std::uniform_real_distribution<float> altitude(0, 120);
        size_t checks = 0, candidates = 0, queries = 0;
        auto probe = [&](Vector3 _origin) {
            checks += Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, _origin);
            candidates += mesh.m_index.Query(_origin.m_x, _origin.m_z, mesh.m_triangles.size()).Size();
            ++queries;
        };
        for (int i = 0; i < 1000; ++i)
            probe({position(generator), altitude(generator), position(generator)});
        for (size_t id = 0; id < mesh.m_triangles.size(); id += std::max(size_t(1), mesh.m_triangles.size() / 600))
        {
            const auto& triangle = mesh.m_triangles[id];
            const Vector3 center = (triangle.m_v1 + triangle.m_v2 + triangle.m_v3) * (1.f / 3.f);
            probe({center.m_x, center.m_y + 2, center.m_z});
            probe({triangle.m_v1.m_x, triangle.m_v1.m_y + 2, triangle.m_v1.m_z});
            probe({std::nextafter(triangle.m_v1.m_x, -INFINITY), triangle.m_v1.m_y + 2, triangle.m_v1.m_z});
            probe({std::nextafter(triangle.m_v1.m_x, INFINITY), triangle.m_v1.m_y + 2, triangle.m_v1.m_z});
        }

        // Concurrent read-only lookups must keep the same IDs and exact float outputs.
        std::vector<std::future<size_t>> workers;
        for (int i = 0; i < 4; ++i)
            workers.push_back(std::async(std::launch::async, [&] {
                return Equivalent(mesh.m_triangles, mesh.m_nodes, mesh.m_index, {0, 30, 0});
            }));
        for (auto& worker : workers)
            checks += worker.get();
        total += checks;
        if (comma)
            std::cout << ',';
        std::cout << "{\"name\":\"" << (height ? "height" : "nav") << "\",\"triangles\":" << mesh.m_triangles.size() << ",\"checks\":" << checks << ",\"cells\":" << mesh.m_index.Cells() << ",\"references\":" << mesh.m_index.References()
                  << ",\"global\":" << mesh.m_index.GlobalCandidates() << ",\"bytes\":" << mesh.m_index.Bytes() << ",\"candidates_per_query\":" << double(candidates) / queries << '}';
        comma = true;
    }

    std::cout << "],\"ok\":true,\"checks\":" << total << "}\n";

    return 0;
}
