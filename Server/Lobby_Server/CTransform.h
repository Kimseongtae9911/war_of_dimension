#pragma once

namespace wod_server {
	class CTransform
	{
	public:
		CTransform();
		~CTransform() {}

		void Reset();
		void Move(const vec3& shift, float height);
		bool CheckDistance(const vec3& pos);

		const vec3& GetLook() { return m_look; }
		const vec3& GetUp() { return m_up; }
		const vec3& GetRight() { return m_right; }
		char GetDir() const { return m_dir; }
		const vec3& GetPos() const { return m_pos; }

		void SetDir(char dir) { m_dir = dir; }
		void SetLook(const vec3& look) { m_look = look; }
		void SetRight(const vec3& right) { m_right = right; }
		void SetPos(const float x, const float y, const float z) { m_pos.x = x, m_pos.y = y, m_pos.z = z; }
		void SetPos(const vec3& pos) { m_pos = pos; }

	private:
		char m_dir = 0;
		vec3 m_look = {};
		vec3 m_up = {};
		vec3 m_right = {};
		vec3 m_pos = {};
	};
}