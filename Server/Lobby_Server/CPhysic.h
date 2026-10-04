#pragma once

namespace wod_server {
	class CPhysic
	{
	public:
		CPhysic();
		~CPhysic() {}

		void SetVelocity(const vec3& v) { m_vel = v; }
		vec3& GetVelocity() { return m_vel; }

		const vec3& CalculateMoveShift(char dir, const vec3& look, const vec3& right);
		void Deceleration(float elapsedTime);

	private:
		vec3 m_vel = {};
		float m_maxVelXZ = 0.0f;
		float m_maxVelY = 0.0f;
		float m_friction = 0.0f;
	};
}