#pragma once
#include <random>

#define RAY_OFFSET 2.0f

namespace wod_server {
	static bool IsFloatEqual(float f1, float f2) {
		return ::abs(f1 - f2) <= FLT_EPSILON;
	}

	static bool IsFloatEqual(float f1, float f2, float epsilon) {
		return ::abs(f1 - f2) <= epsilon;
	}

	class Vector2
	{
	public:
		Vector2() { x = 0; z = 0; };
		constexpr Vector2(float x, float z) noexcept : x(x), z(z) {}

		friend std::ostream& operator<< (std::ostream& os, const Vector2& v)
		{
			return os << "{" << v.x << ", " << v.z << "}";
		}

		friend Vector2  operator -(const Vector2& l, const Vector2& r)
		{
			return Vector2(l.x - r.x, l.z - r.z);
		}

		static Vector2 Normalize(Vector2& vec)
		{
			float length = vec.Length();
			if (length == 0)
				return Vector2(0, 0);
			return Vector2(vec.x / length, vec.z / length);
		}
		static Vector2 Normalize(Vector2&& vec)
		{
			float length = vec.Length();
			if (length == 0)
				return Vector2(0, 0);
			return Vector2(vec.x / length, vec.z / length);
		}

		const float Length()
		{
			return sqrtf(x * x + z * z);
		}

		float x, z;
	};

	class Vector3 {
	public:
		Vector3() { x = 0; y = 0; z = 0; }
		Vector3(const DirectX::XMFLOAT3& pos) { x = pos.x; y = pos.y; z = pos.z; }
		constexpr Vector3(float x, float y, float z) noexcept : x(x), y(y), z(z) {}

		friend std::ostream& operator<< (std::ostream& os, const Vector3& v) {
			return os << "{" << v.x << ", " << v.y << ", " << v.z << "}";
		}

		friend Vector3 operator +(const Vector3& l, const Vector3& r) {		
			return Vector3(l.x + r.x, l.y + r.y, l.z + r.z);
		}

		friend Vector3  operator -(const Vector3& l, const Vector3& r) {
			return Vector3(l.x - r.x, l.y - r.y, l.z - r.z);
		}

		friend void operator +=(Vector3& l, const Vector3& r) {
			l.x += r.x;
			l.y += r.y;
			l.z += r.z;
		}

		friend void operator -=(Vector3& l, const Vector3& r) {
			l.x -= r.x;
			l.y -= r.y;
			l.z -= r.z;
		}

		friend bool operator==(const Vector3& l, const Vector3& r) {
			return (IsFloatEqual(l.x, r.x) && IsFloatEqual(l.y, r.y) && IsFloatEqual(l.z, r.z));
		}

		friend bool operator!=(const Vector3& l, const Vector3& r) {		
			return (!IsFloatEqual(l.x, r.x) || !IsFloatEqual(l.y, r.y) || !IsFloatEqual(l.z, r.z));
		}

		friend Vector3 operator*(Vector3& l, const float r) {
			return Vector3(l.x * r, l.y * r, l.z * r);
		}

		friend Vector3 operator*(const Vector3& l, const float r) {
			return Vector3(l.x * r, l.y * r, l.z * r);
		}

		friend Vector3 operator*(const Vector3& l, const Vector3& r) {
			return Vector3(l.x * r.x, l.y * r.y, l.z * r.z);
		}

		friend Vector3 operator/(Vector3& l, const float r) {
			return Vector3(l.x / r, l.y / r, l.z / r);
		}

		static Vector3 Add(Vector3& vec1, Vector3& vec2, float scalar) {
			return vec1 + (vec2 * scalar);
		}

		static Vector3 Add(Vector3& vec1, const Vector3& vec2, float scalar) {
			return vec1 + (vec2 * scalar);
		}

		static Vector3 Add(Vector3& vec1, Vector3&& vec2) {		
			return vec1 + vec2;
		}

		static Vector3 Normalize(const Vector3& vec) {
			float length = vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(vec.x / length, vec.y / length, vec.z / length);
		}

		static Vector3 Normalize(Vector3& vec) {
			float length = vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(vec.x / length, vec.y / length, vec.z / length);
		}

		static Vector3 Normalize(Vector3&& vec) {
			float length = vec.Length();
			if (length == 0)
				return Vector3(0, 0, 0);
			return Vector3(vec.x / length, vec.y / length, vec.z / length);
		}

		static Vector3 Lerp(const Vector3& start, const Vector3& end, float t) {
			t = std::clamp(t, 0.0f, 1.0f);

			return start * (1.0f - t) + end * t;
		}

		static Vector3 Reflect(const Vector3& vector, const Vector3& normal) {
			return vector - normal * 2.0f * vector.Dot(normal);
		}

		Vector3 Cross(Vector3& vec) {
			return Vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, x * vec.y - y * vec.x);
		}

		Vector3 Cross(Vector3&& vec) {		
			return Vector3(y * vec.z - z * vec.y, z * vec.x - x * vec.z, x * vec.y - y * vec.x);
		}

		float Dot(const Vector3& vec) const {
			return (x * vec.x) + (y * vec.y) + (z * vec.z);
		}

		const float Length() const {
			return sqrtf(x * x + y * y + z * z);
		}

		float Magnitude(const Vector3& v) {
			return sqrt((x - v.x) * (x - v.x) + (y - v.y) * (y - v.y) + (z - v.z) * (z - v.z));
		}

		float x, y, z;
	};

	class Triangle
	{
	public:
		Vector3 v1, v2, v3;
		int id = -1;
		Triangle() { v1 = {}; v2 = {}; v3 = {}; };
		Triangle(Vector3 v1, Vector3 v2, Vector3 v3) { this->v1 = v1; this->v2 = v2; this->v3 = v3; }
		Triangle(Vector3 v1, Vector3 v2, Vector3 v3, int id) { this->v1 = v1; this->v2 = v2; this->v3 = v3; this->id = id; }

		const Vector3& GetVertex(int index) const {
			switch (index) {
			case 0:
				return v1;
				break;
			case 1:
				return v2;
				break;
			case 2: 
				return v3;
				break;
			default:
				return v1;
				break;
			}
		}

		Vector3 GetEdge(int index) const {
			Vector3 v1 = GetVertex(index);
			Vector3 v2 = GetVertex((index + 1) % 3);

			return v2 - v1;
		}

		bool ContainsEdge(const Vector3& v1, const Vector3& v2) const {
			return (v1 == v2) && ((v1 == v1 && v2 == v2) || (v1 == v2 && v2 == v1));
		}
	};

	class Node
	{
	public:
		Triangle triangle;
		std::vector<Node*> adjacentNodes;

		Node(const Triangle& triangle) : triangle(triangle) {}
	};

	class Ray
	{
	public:
		Ray() { origin = {0.0f, 0.0f, 0.0f}; direction = { 0.0f, -1.0f, 0.0f }; }
		Ray(Vector3& pos) { origin = pos; direction = { 0.0f, -1.0f, 0.0f }; }
		Ray(Vector3& pos, Vector3& dir) { origin = pos; direction = dir; }
		Vector3 origin, direction;

		bool RayCast(const std::vector<Triangle>& triangles, float& distance) const
		{
			for (const Triangle& triangle : triangles) {
				if (triangle.v1.y < 1.5f || triangle.v2.y < 1.5f || triangle.v3.y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.v2 - triangle.v1).Cross(triangle.v3 - triangle.v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.v1 - origin).Dot(normal) / direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = origin + direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.v2 - triangle.v1;
					Vector3 edge2 = triangle.v3 - triangle.v1;
					Vector3 pointToV1 = point - triangle.v1;
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
						if (::abs(direction.y * d) - RAY_OFFSET >= 0.8f) {
							distance = origin.y - RAY_OFFSET;
						}
						else {
							distance = origin.y + direction.y * d;
						}
						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Triangle>& triangles, float& distance, bool skill) const
		{
			for (const Triangle& triangle : triangles) {
				if (triangle.v1.y < 1.5f || triangle.v2.y < 1.5f || triangle.v3.y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.v2 - triangle.v1).Cross(triangle.v3 - triangle.v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.v1 - origin).Dot(normal) / direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = origin + direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.v2 - triangle.v1;
					Vector3 edge2 = triangle.v3 - triangle.v1;
					Vector3 pointToV1 = point - triangle.v1;
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
						distance = origin.y + direction.y * d;

						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Node*>& nodes, float& distance, int& nodeNum) const
		{
			for (const Node* node : nodes) {
				Triangle triangle = node->triangle;
				if (triangle.v1.y < 1.5f || triangle.v2.y < 1.5f || triangle.v3.y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.v2 - triangle.v1).Cross(triangle.v3 - triangle.v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.v1 - origin).Dot(normal) / direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = origin + direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.v2 - triangle.v1;
					Vector3 edge2 = triangle.v3 - triangle.v1;
					Vector3 pointToV1 = point - triangle.v1;
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
						if (::abs(direction.y * d) - RAY_OFFSET >= 0.6f) {
							distance = origin.y - RAY_OFFSET;
						}
						else {
							distance = origin.y + direction.y * d;
						}
						nodeNum = triangle.id;
						return true;
					}
				}
			}

			return false;
		}

		bool RayCast(const std::vector<Node*>& nodes, float& distance, int& nodeNum, bool skill) const
		{
			for (const Node* node : nodes) {
				Triangle triangle = node->triangle;
				if (triangle.v1.y < 1.5f || triangle.v2.y < 1.5f || triangle.v3.y < 1.5f) {
					continue;
				}
				Vector3 normal = Vector3::Normalize((triangle.v2 - triangle.v1).Cross(triangle.v3 - triangle.v1));

				// Calculate the distance from the ray origin to the plane of the triangle
				float d = (triangle.v1 - origin).Dot(normal) / direction.Dot(normal);

				// Check if the ray intersects the plane of the triangle
				if (d >= 0.0f) {
					// Calculate the point of intersection on the plane of the triangle
					Vector3 point = origin + direction * d;

					// Check if the point of intersection is inside the triangle
					Vector3 edge1 = triangle.v2 - triangle.v1;
					Vector3 edge2 = triangle.v3 - triangle.v1;
					Vector3 pointToV1 = point - triangle.v1;
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
						distance = origin.y + direction.y * d;
						nodeNum = triangle.id;
						return true;
					}
				}
			}

			return false;
		}
	};

	static float DistanceXZ(const Vector3& pos1, const Vector3& pos2)
	{
		return ::sqrtf(::powf(pos1.x - pos2.x, 2) + ::powf(pos1.z - pos2.z, 2));
	}

	static float DistanceXYZ(const Vector3& pos1, const Vector3& pos2)
	{
		return ::sqrtf(::powf(pos1.x - pos2.x, 2) + ::powf(pos1.y - pos2.y, 2) + ::powf(pos1.z - pos2.z, 2));
	}

	static float DistanceXYZ(const Node* node1, const Node* node2)
	{
		const Triangle& triangle1 = node1->triangle;
		const Triangle& triangle2 = node2->triangle;

		Vector3 centroid1 = Vector3((node1->triangle.v1.x + node1->triangle.v2.x + node1->triangle.v3.x) / 3.0f,
									(node1->triangle.v1.y + node1->triangle.v2.y + node1->triangle.v3.y) / 3.0f,
									(node1->triangle.v1.z + node1->triangle.v2.z + node1->triangle.v3.z) / 3.0f);

		Vector3 centroid2 = Vector3((node2->triangle.v1.x + node2->triangle.v2.x + node2->triangle.v3.x) / 3.0f,
									(node2->triangle.v1.y + node2->triangle.v2.y + node2->triangle.v3.y) / 3.0f,
									(node2->triangle.v1.z + node2->triangle.v2.z + node2->triangle.v3.z) / 3.0f);

		return DistanceXYZ(centroid1, centroid2);
	}
}
using vec2 = wod_server::Vector2;
using vec3 = wod_server::Vector3;

