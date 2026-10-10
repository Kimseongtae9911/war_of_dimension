#pragma once
#include <random>

#define RAY_OFFSET 2.0f

namespace wod_server {
	static bool IsFloatEqual(float _f1, float _f2) {
		return ::abs(_f1 - _f2) <= FLT_EPSILON;
	}

	static bool IsFloatEqual(float _f1, float _f2, float _epsilon) {
		return ::abs(_f1 - _f2) <= _epsilon;
	}

	class Vector2
	{
	public:
		Vector2() { m_x = 0; m_z = 0; };
		constexpr Vector2(float _x, float _z) noexcept : m_x(_x), m_z(_z) {}

		friend std::ostream& operator<< (std::ostream& _os, const Vector2& _v)
		{
			return _os << "{" << _v.m_x << ", " << _v.m_z << "}";
		}

		friend Vector2  operator -(const Vector2& _l, const Vector2& _r)
		{
			return Vector2(_l.m_x - _r.m_x, _l.m_z - _r.m_z);
		}

		static Vector2 Normalize(Vector2& _vec)
		{
			float length = _vec.Length();
			if (length == 0)
				return Vector2(0, 0);
			return Vector2(_vec.m_x / length, _vec.m_z / length);
		}
		static Vector2 Normalize(Vector2&& _vec)
		{
			float length = _vec.Length();
			if (length == 0)
				return Vector2(0, 0);
			return Vector2(_vec.m_x / length, _vec.m_z / length);
		}

		const float Length()
		{
			return sqrtf(m_x * m_x + m_z * m_z);
		}

		float m_x, m_z;
	};

	class Vector3 {
	public:
		Vector3() { m_x = 0; m_y = 0; m_z = 0; }
		Vector3(const DirectX::XMFLOAT3& _pos) { m_x = _pos.x; m_y = _pos.y; m_z = _pos.z; }
		constexpr Vector3(float _x, float _y, float _z) noexcept : m_x(_x), m_y(_y), m_z(_z) {}

		friend std::ostream& operator<< (std::ostream& _os, const Vector3& _v) {
			return _os << "{" << _v.m_x << ", " << _v.m_y << ", " << _v.m_z << "}";
		}

		friend Vector3 operator +(const Vector3& _l, const Vector3& _r) {
			return Vector3(_l.m_x + _r.m_x, _l.m_y + _r.m_y, _l.m_z + _r.m_z);
		}

		friend Vector3  operator -(const Vector3& _l, const Vector3& _r) {
			return Vector3(_l.m_x - _r.m_x, _l.m_y - _r.m_y, _l.m_z - _r.m_z);
		}

		friend void operator +=(Vector3& _l, const Vector3& _r) {
			_l.m_x += _r.m_x;
			_l.m_y += _r.m_y;
			_l.m_z += _r.m_z;
		}

		friend void operator -=(Vector3& _l, const Vector3& _r) {
			_l.m_x -= _r.m_x;
			_l.m_y -= _r.m_y;
			_l.m_z -= _r.m_z;
		}

		friend bool operator==(const Vector3& _l, const Vector3& _r) {
			return (IsFloatEqual(_l.m_x, _r.m_x) && IsFloatEqual(_l.m_y, _r.m_y) && IsFloatEqual(_l.m_z, _r.m_z));
		}

		friend bool operator!=(const Vector3& _l, const Vector3& _r) {
			return (!IsFloatEqual(_l.m_x, _r.m_x) || !IsFloatEqual(_l.m_y, _r.m_y) || !IsFloatEqual(_l.m_z, _r.m_z));
		}

		friend Vector3 operator*(Vector3& _l, const float _r) {
			return Vector3(_l.m_x * _r, _l.m_y * _r, _l.m_z * _r);
		}

		friend Vector3 operator*(const Vector3& _l, const float _r) {
			return Vector3(_l.m_x * _r, _l.m_y * _r, _l.m_z * _r);
		}

		friend Vector3 operator*(const Vector3& _l, const Vector3& _r) {
			return Vector3(_l.m_x * _r.m_x, _l.m_y * _r.m_y, _l.m_z * _r.m_z);
		}

		friend Vector3 operator/(Vector3& _l, const float _r) {
			return Vector3(_l.m_x / _r, _l.m_y / _r, _l.m_z / _r);
		}

		static Vector3 Add(Vector3& _vec1, Vector3& _vec2, float _scalar) {
			return _vec1 + (_vec2 * _scalar);
		}

		static Vector3 Add(Vector3& _vec1, const Vector3& _vec2, float _scalar) {
			return _vec1 + (_vec2 * _scalar);
		}

		static Vector3 Add(Vector3& _vec1, Vector3&& _vec2) {
			return _vec1 + _vec2;
		}

		static Vector3 Normalize(const Vector3& _vec) {
			float length = _vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(_vec.m_x / length, _vec.m_y / length, _vec.m_z / length);
		}

		static Vector3 Normalize(Vector3& _vec) {
			float length = _vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(_vec.m_x / length, _vec.m_y / length, _vec.m_z / length);
		}

		static Vector3 Normalize(Vector3&& _vec) {
			float length = _vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(_vec.m_x / length, _vec.m_y / length, _vec.m_z / length);
		}

		static Vector3 Lerp(const Vector3& _start, const Vector3& _end, float _t) {
			_t = std::clamp(_t, 0.0f, 1.0f);

			return _start * (1.0f - _t) + _end * _t;
		}

		static Vector3 Reflect(const Vector3& _vector, const Vector3& _normal) {
			return _vector - _normal * 2.0f * _vector.Dot(_normal);
		}

		Vector3 Cross(Vector3& _vec) {
			return Vector3(m_y * _vec.m_z - m_z * _vec.m_y, m_z * _vec.m_x - m_x * _vec.m_z, m_x * _vec.m_y - m_y * _vec.m_x);
		}

		Vector3 Cross(Vector3&& _vec) {
			return Vector3(m_y * _vec.m_z - m_z * _vec.m_y, m_z * _vec.m_x - m_x * _vec.m_z, m_x * _vec.m_y - m_y * _vec.m_x);
		}

		float Dot(const Vector3& _vec) const {
			return (m_x * _vec.m_x) + (m_y * _vec.m_y) + (m_z * _vec.m_z);
		}

		const float Length() const {
			return sqrtf(m_x * m_x + m_y * m_y + m_z * m_z);
		}

		float Magnitude(const Vector3& _v) {
			return sqrt((m_x - _v.m_x) * (m_x - _v.m_x) + (m_y - _v.m_y) * (m_y - _v.m_y) + (m_z - _v.m_z) * (m_z - _v.m_z));
		}

		float m_x, m_y, m_z;
	};

	class Triangle
	{
	public:
		Vector3 m_v1, m_v2, m_v3;
		int m_id = -1;
		Triangle() { m_v1 = {}; m_v2 = {}; m_v3 = {}; };
		Triangle(Vector3 _v1, Vector3 _v2, Vector3 _v3) { this->m_v1 = _v1; this->m_v2 = _v2; this->m_v3 = _v3; }
		Triangle(Vector3 _v1, Vector3 _v2, Vector3 _v3, int _id) { this->m_v1 = _v1; this->m_v2 = _v2; this->m_v3 = _v3; this->m_id = _id; }

		const Vector3& GetVertex(int _index) const {
			switch (_index) {
			case 0:
				return m_v1;
				break;
			case 1:
				return m_v2;
				break;
			case 2:
				return m_v3;
				break;
			default:
				return m_v1;
				break;
			}
		}

		Vector3 GetEdge(int _index) const {
			Vector3 v1 = GetVertex(_index);
			Vector3 v2 = GetVertex((_index + 1) % 3);

			return v2 - v1;
		}

		bool ContainsEdge(const Vector3& _v1, const Vector3& _v2) const {
			return (_v1 == _v2) && ((_v1 == _v1 && _v2 == _v2) || (_v1 == _v2 && _v2 == _v1));
		}
	};

	class Node
	{
	public:
		Triangle m_triangle;
		std::vector<Node*> m_adjacentNodes;

		Node(const Triangle& _triangle) : m_triangle(_triangle) {}
	};

	class Ray
	{
	public:
		Ray() { m_origin = {0.0f, 0.0f, 0.0f}; m_direction = { 0.0f, -1.0f, 0.0f }; }
		Ray(Vector3& _pos) { m_origin = _pos; m_direction = { 0.0f, -1.0f, 0.0f }; }
		Ray(Vector3& _pos, Vector3& _dir) { m_origin = _pos; m_direction = _dir; }
		Vector3 m_origin, m_direction;

		bool RayCast(const std::vector<Triangle>& _triangles, float& _distance) const
		{
			for (const Triangle& triangle : _triangles) {
				if (triangle.m_v1.m_y < 1.5f || triangle.m_v2.m_y < 1.5f || triangle.m_v3.m_y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.m_v2 - triangle.m_v1).Cross(triangle.m_v3 - triangle.m_v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.m_v1 - m_origin).Dot(normal) / m_direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = m_origin + m_direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.m_v2 - triangle.m_v1;
					Vector3 edge2 = triangle.m_v3 - triangle.m_v1;
					Vector3 pointToV1 = point - triangle.m_v1;
					float dot11 = edge1.Dot(edge1);
					float dot12 = edge1.Dot(edge2);
					float dot22 = edge2.Dot(edge2);
					float dotP1 = pointToV1.Dot(edge1);
					float dotP2 = pointToV1.Dot(edge2);
					float invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);
					float u = (dot22 * dotP1 - dot12 * dotP2) * invDenom;
					float v = (dot11 * dotP2 - dot12 * dotP1) * invDenom;

					// Check if the point of intersection is inside the triangle
					if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
						if (::abs(m_direction.m_y * d) - RAY_OFFSET >= 0.8f) {
							_distance = m_origin.m_y - RAY_OFFSET;
						}
						else {
							_distance = m_origin.m_y + m_direction.m_y * d;
						}
						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Triangle>& _triangles, float& _distance, bool _skill) const
		{
			for (const Triangle& triangle : _triangles) {
				if (triangle.m_v1.m_y < 1.5f || triangle.m_v2.m_y < 1.5f || triangle.m_v3.m_y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.m_v2 - triangle.m_v1).Cross(triangle.m_v3 - triangle.m_v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.m_v1 - m_origin).Dot(normal) / m_direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = m_origin + m_direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.m_v2 - triangle.m_v1;
					Vector3 edge2 = triangle.m_v3 - triangle.m_v1;
					Vector3 pointToV1 = point - triangle.m_v1;
					float dot11 = edge1.Dot(edge1);
					float dot12 = edge1.Dot(edge2);
					float dot22 = edge2.Dot(edge2);
					float dotP1 = pointToV1.Dot(edge1);
					float dotP2 = pointToV1.Dot(edge2);
					float invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);
					float u = (dot22 * dotP1 - dot12 * dotP2) * invDenom;
					float v = (dot11 * dotP2 - dot12 * dotP1) * invDenom;

					// Check if the point of intersection is inside the triangle
					if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
						_distance = m_origin.m_y + m_direction.m_y * d;

						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Node*>& _nodes, float& _distance, int& _nodeNum) const
		{
			for (const Node* node : _nodes) {
				Triangle triangle = node->m_triangle;
				if (triangle.m_v1.m_y < 1.5f || triangle.m_v2.m_y < 1.5f || triangle.m_v3.m_y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.m_v2 - triangle.m_v1).Cross(triangle.m_v3 - triangle.m_v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.m_v1 - m_origin).Dot(normal) / m_direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = m_origin + m_direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.m_v2 - triangle.m_v1;
					Vector3 edge2 = triangle.m_v3 - triangle.m_v1;
					Vector3 pointToV1 = point - triangle.m_v1;
					float dot11 = edge1.Dot(edge1);
					float dot12 = edge1.Dot(edge2);
					float dot22 = edge2.Dot(edge2);
					float dotP1 = pointToV1.Dot(edge1);
					float dotP2 = pointToV1.Dot(edge2);
					float invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);
					float u = (dot22 * dotP1 - dot12 * dotP2) * invDenom;
					float v = (dot11 * dotP2 - dot12 * dotP1) * invDenom;

					// Check if the point of intersection is inside the triangle
					if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
						if (::abs(m_direction.m_y * d) - RAY_OFFSET >= 0.6f) {
							_distance = m_origin.m_y - RAY_OFFSET;
						}
						else {
							_distance = m_origin.m_y + m_direction.m_y * d;
						}
						_nodeNum = triangle.m_id;
						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Node*>& _nodes, float& _distance, int& _nodeNum, bool _skill) const
		{
			for (const Node* node : _nodes) {
				Triangle triangle = node->m_triangle;
				if (triangle.m_v1.m_y < 1.5f || triangle.m_v2.m_y < 1.5f || triangle.m_v3.m_y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.m_v2 - triangle.m_v1).Cross(triangle.m_v3 - triangle.m_v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.m_v1 - m_origin).Dot(normal) / m_direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = m_origin + m_direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.m_v2 - triangle.m_v1;
					Vector3 edge2 = triangle.m_v3 - triangle.m_v1;
					Vector3 pointToV1 = point - triangle.m_v1;
					float dot11 = edge1.Dot(edge1);
					float dot12 = edge1.Dot(edge2);
					float dot22 = edge2.Dot(edge2);
					float dotP1 = pointToV1.Dot(edge1);
					float dotP2 = pointToV1.Dot(edge2);
					float invDenom = 1.0f / (dot11 * dot22 - dot12 * dot12);
					float u = (dot22 * dotP1 - dot12 * dotP2) * invDenom;
					float v = (dot11 * dotP2 - dot12 * dotP1) * invDenom;

					// Check if the point of intersection is inside the triangle
					if (u >= 0.0f && v >= 0.0f && u + v <= 1.0f) {
						_distance = m_origin.m_y + m_direction.m_y * d;
						_nodeNum = triangle.m_id;
						return true;
					}
				}
			}

			return false;
		}
	};

	static float DistanceXZ(const Vector3& _pos1, const Vector3& _pos2)
	{
		return ::sqrtf(::powf(_pos1.m_x - _pos2.m_x, 2) + ::powf(_pos1.m_z - _pos2.m_z, 2));
	}

	static float DistanceXYZ(const Vector3& _pos1, const Vector3& _pos2)
	{
		return ::sqrtf(::powf(_pos1.m_x - _pos2.m_x, 2) + ::powf(_pos1.m_y - _pos2.m_y, 2) + ::powf(_pos1.m_z - _pos2.m_z, 2));
	}

	static float DistanceXYZ(const Node* _node1, const Node* _node2)
	{
		const Triangle& triangle1 = _node1->m_triangle;
		const Triangle& triangle2 = _node2->m_triangle;

		Vector3 centroid1 = Vector3((_node1->m_triangle.m_v1.m_x + _node1->m_triangle.m_v2.m_x + _node1->m_triangle.m_v3.m_x) / 3.0f,
									(_node1->m_triangle.m_v1.m_y + _node1->m_triangle.m_v2.m_y + _node1->m_triangle.m_v3.m_y) / 3.0f,
									(_node1->m_triangle.m_v1.m_z + _node1->m_triangle.m_v2.m_z + _node1->m_triangle.m_v3.m_z) / 3.0f);

		Vector3 centroid2 = Vector3((_node2->m_triangle.m_v1.m_x + _node2->m_triangle.m_v2.m_x + _node2->m_triangle.m_v3.m_x) / 3.0f,
									(_node2->m_triangle.m_v1.m_y + _node2->m_triangle.m_v2.m_y + _node2->m_triangle.m_v3.m_y) / 3.0f,
									(_node2->m_triangle.m_v1.m_z + _node2->m_triangle.m_v2.m_z + _node2->m_triangle.m_v3.m_z) / 3.0f);

		return DistanceXYZ(centroid1, centroid2);
	}
}
using vec2 = wod_server::Vector2;
using vec3 = wod_server::Vector3;

