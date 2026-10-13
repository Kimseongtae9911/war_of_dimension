#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <type_traits>
#include <vector>

namespace wod_server
{
// Read-only XZ broad phase. IDs always refer to the original mesh order.
class TerrainSpatialIndex
{
  public:
    class Indices
    {
      public:
        struct Iterator
        {
            size_t m_position = 0;
            size_t m_local = 0;
            size_t m_global = 0;
            const Indices *m_range;

            size_t operator*() const
            {
                if (!m_range->m_indexed)
                    return m_position;

                const size_t local = m_local < m_range->m_local.size() ? m_range->m_local[m_local] : (std::numeric_limits<size_t>::max)();
                const size_t global = m_global < m_range->m_global.size() ? m_range->m_global[m_global] : (std::numeric_limits<size_t>::max)();

                return (std::min)(local, global);
            }

            Iterator& operator++()
            {
                if (m_range->m_indexed)
                {
                    if (m_local < m_range->m_local.size() && (m_global == m_range->m_global.size() || m_range->m_local[m_local] < m_range->m_global[m_global]))
                    {
                        ++m_local;
                    }
                    else
                    {
                        ++m_global;
                    }
                }

                ++m_position;

                return *this;
            }

            bool operator!=(const Iterator& _other) const
            {
                return m_position != _other.m_position;
            }
        };

        static Indices Linear(size_t _count)
        {
            return Indices(_count);
        }

        Iterator begin() const
        {
            return {0, 0, 0, this};
        }

        Iterator end() const
        {
            return {m_count, 0, 0, this};
        }

        size_t Size() const
        {
            return m_count;
        }

      private:
        friend class TerrainSpatialIndex;

        explicit Indices(size_t _count) : m_count(_count)
        {
        }

        Indices(std::span<const size_t> _local, std::span<const size_t> _global) : m_count(_local.size() + _global.size()), m_local(_local), m_global(_global), m_indexed(true)
        {
        }

        size_t m_count;
        std::span<const size_t> m_local;
        std::span<const size_t> m_global;
        bool m_indexed = false;
    };

    void Reset()
    {
        *this = TerrainSpatialIndex();
    }

    template <class GetTriangle> bool Build(size_t _count, GetTriangle _triangle, float _cellSize = 4.0f)
    {
        Reset();
        if (!std::isfinite(_cellSize) || _cellSize <= 0 || _count > MaxReferences)
            return false;

        struct Bounds
        {
            double m_minX, m_minZ, m_maxX, m_maxZ;
            bool m_global = false;
            bool m_skip = false;
        };

        std::vector<Bounds> bounds;
        bounds.reserve(_count);
        double minX = 0, minZ = 0, maxX = 0, maxZ = 0;
        bool first = true;
        for (size_t id = 0; id < _count; ++id)
        {
            const auto& triangle = _triangle(id);
            const auto& a = triangle.m_v1;
            const auto& b = triangle.m_v2;
            const auto& c = triangle.m_v3;
            if (!std::isfinite(a.m_x) || !std::isfinite(a.m_y) || !std::isfinite(a.m_z) || !std::isfinite(b.m_x) || !std::isfinite(b.m_y) || !std::isfinite(b.m_z) || !std::isfinite(c.m_x) || !std::isfinite(c.m_y) || !std::isfinite(c.m_z))
                return false;

            // The original height filter rejects these. A vertical ray cannot hit
            // a face with exactly zero normal.y (d becomes NaN/Inf and barycentrics reject).
            const auto normal = std::remove_cvref_t<decltype(a)>::Normalize((b - a).Cross(c - a));
            if (a.m_y < 1.5f || b.m_y < 1.5f || c.m_y < 1.5f || normal.m_y == 0)
            {
                bounds.push_back({0, 0, 0, 0, false, true});
                continue;
            }

            const auto e1 = b - a;
            const auto e2 = c - a;
            const double dot11 = e1.Dot(e1), dot12 = e1.Dot(e2), dot22 = e2.Dot(e2);
            const double determinant = dot11 * dot22 - dot12 * dot12;
            const double condition = determinant > 0 ? dot11 * dot22 / determinant : std::numeric_limits<double>::infinity();
            const double scale = (std::max)({1.0, std::abs(double(a.m_x)), std::abs(double(a.m_z)), std::abs(double(b.m_x)), std::abs(double(b.m_z)), std::abs(double(c.m_x)), std::abs(double(c.m_z)), std::sqrt(dot11), std::sqrt(dot22)});
            const double crossY = double(e1.m_z) * e2.m_x - double(e1.m_x) * e2.m_z;
            const double crossLength = std::sqrt((std::max)(0.0, determinant));
            // Numerically unstable/vertical faces remain global candidates, even outside the grid.
            const bool global = !std::isfinite(condition) || condition > 256 || std::abs(crossY) < crossLength * 0.0001 || scale > 65536;
            const double pad = global ? 0 : 0.01 + 64 * std::numeric_limits<float>::epsilon() * scale * condition;
            Bounds box{(std::min)({double(a.m_x), double(b.m_x), double(c.m_x)}) - pad, (std::min)({double(a.m_z), double(b.m_z), double(c.m_z)}) - pad, (std::max)({double(a.m_x), double(b.m_x), double(c.m_x)}) + pad,
                       (std::max)({double(a.m_z), double(b.m_z), double(c.m_z)}) + pad, global};
            bounds.push_back(box);
            if (global)
                continue;

            minX = first ? box.m_minX : (std::min)(minX, box.m_minX);
            minZ = first ? box.m_minZ : (std::min)(minZ, box.m_minZ);
            maxX = first ? box.m_maxX : (std::max)(maxX, box.m_maxX);
            maxZ = first ? box.m_maxZ : (std::max)(maxZ, box.m_maxZ);
            first = false;
        }

        const double columns = std::floor((maxX - minX) / _cellSize) + 1;
        const double rows = std::floor((maxZ - minZ) / _cellSize) + 1;
        if (!std::isfinite(columns * rows) || columns * rows > MaxCells)
            return false;

        m_minX = minX;
        m_minZ = minZ;
        m_cellSize = _cellSize;
        m_columns = static_cast<size_t>(columns);
        m_rows = static_cast<size_t>(rows);
        m_offsets.resize(m_columns * m_rows + 1);
        auto cells = [&](const Bounds& _box, auto _visit) {
            const size_t x0 = Cell(_box.m_minX, m_minX, m_columns), x1 = Cell(_box.m_maxX, m_minX, m_columns);
            const size_t z0 = Cell(_box.m_minZ, m_minZ, m_rows), z1 = Cell(_box.m_maxZ, m_minZ, m_rows);
            for (size_t z = z0; z <= z1; ++z)
                for (size_t x = x0; x <= x1; ++x)
                    _visit(z * m_columns + x);
        };
        size_t references = 0;
        for (size_t id = 0; id < bounds.size(); ++id)
        {
            if (bounds[id].m_skip)
                continue;

            if (bounds[id].m_global)
            {
                m_global.push_back(id);
                continue;
            }

            cells(bounds[id], [&](size_t _cell) {
                ++m_offsets[_cell + 1];
                ++references;
            });
            if (references > MaxReferences)
            {
                Reset();

                return false;
            }
        }

        for (size_t cell = 1; cell < m_offsets.size(); ++cell)
            m_offsets[cell] += m_offsets[cell - 1];
        m_indices.resize(references);
        auto cursor = m_offsets;
        for (size_t id = 0; id < bounds.size(); ++id)
            if (!bounds[id].m_global && !bounds[id].m_skip)
                cells(bounds[id], [&](size_t _cell) {
                    m_indices[cursor[_cell]++] = id;
                });
        m_sourceCount = _count;
        m_ready = true;

        return true;
    }

    Indices Query(float _x, float _z, size_t _sourceCount) const
    {
        if (!m_ready || _sourceCount != m_sourceCount || !std::isfinite(_x) || !std::isfinite(_z))
            return Indices::Linear(_sourceCount);

        const double x = std::floor((double(_x) - m_minX) / m_cellSize), z = std::floor((double(_z) - m_minZ) / m_cellSize);
        if (x < 0 || z < 0 || x >= m_columns || z >= m_rows)
            return Indices({}, m_global);

        const size_t cell = static_cast<size_t>(z) * m_columns + static_cast<size_t>(x);

        return Indices(std::span<const size_t>(m_indices).subspan(m_offsets[cell], m_offsets[cell + 1] - m_offsets[cell]), m_global);
    }

    bool Ready() const
    {
        return m_ready;
    }

    size_t Cells() const
    {
        return m_columns * m_rows;
    }

    size_t References() const
    {
        return m_indices.size();
    }

    size_t GlobalCandidates() const
    {
        return m_global.size();
    }

    size_t Bytes() const
    {
        return (m_indices.capacity() + m_offsets.capacity() + m_global.capacity()) * sizeof(size_t);
    }

  private:
    size_t Cell(double _value, double _minimum, size_t _dimension) const
    {
        return static_cast<size_t>(std::clamp(std::floor((_value - _minimum) / m_cellSize), 0.0, double(_dimension - 1)));
    }

    static constexpr size_t MaxCells = 1024 * 1024;
    static constexpr size_t MaxReferences = 8 * 1024 * 1024;
    double m_minX = 0, m_minZ = 0, m_cellSize = 4;
    size_t m_columns = 0, m_rows = 0, m_sourceCount = 0;
    std::vector<size_t> m_offsets, m_indices, m_global;
    bool m_ready = false;
};
}
