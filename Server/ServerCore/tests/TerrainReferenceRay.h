// Frozen linear oracle: main 3e34e71a Server/Game_Server/MathUtil.h Ray.
// Keep the original arithmetic and first-hit order for compatibility checks.
#pragma once
namespace wod_server {
	class ReferenceRay
	{
	public:
		ReferenceRay() { m_origin = {0.0f, 0.0f, 0.0f}; m_direction = { 0.0f, -1.0f, 0.0f }; }
		ReferenceRay(Vector3& _pos) { m_origin = _pos; m_direction = { 0.0f, -1.0f, 0.0f }; }
		ReferenceRay(Vector3& _pos, Vector3& _dir) { m_origin = _pos; m_direction = _dir; }
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

}
