#pragma once

namespace wod_server {
	class CTransform
	{
	public:
		CTransform();
		~CTransform() {}

		void Reset();
		void Move(const vec3& _shift, float _height);
		bool CheckDistance(const vec3& _pos);

		const vec3& GetLook() { return m_look; }
		const vec3& GetUp() { return m_up; }
		const vec3& GetRight() { return m_right; }
		char GetDir() const { return m_dir; }
		const vec3& GetPos() const { return m_pos; }

		void SetDir(char _dir) { m_dir = _dir; }
		void SetLook(const vec3& _look) { m_look = _look; }
		void SetRight(const vec3& _right) { m_right = _right; }
		void SetPos(const float _x, const float _y, const float _z) { m_pos.m_x = _x, m_pos.m_y = _y, m_pos.m_z = _z; }
		void SetPos(const vec3& _pos) { m_pos = _pos; }

	private:
		char m_dir = 0;
		vec3 m_look = {};
		vec3 m_up = {};
		vec3 m_right = {};
		vec3 m_pos = {};
	};
}