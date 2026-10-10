#pragma once

namespace wod_server {
	class CPhysic
	{
	public:
		CPhysic();
		~CPhysic() {}

		void SetVelocity(const vec3& _v) { m_vel = _v; }
		vec3& GetVelocity() { return m_vel; }

		const vec3& CalculateMoveShift(char _dir, const vec3& _look, const vec3& _right);
		void Deceleration(float _elapsedTime);

	private:
		vec3 m_vel = {};
		float m_maxVelXZ = 0.0f;
		float m_maxVelY = 0.0f;
		float m_friction = 0.0f;
	};
}