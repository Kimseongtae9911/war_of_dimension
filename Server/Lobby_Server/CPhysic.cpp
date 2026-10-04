#include "pch.h"
#include "CPhysic.h"

namespace wod_server {
	CPhysic::CPhysic()
	{
		m_maxVelXZ = PLAYER_MAX_VELXZ + 4.0f;
		m_maxVelY = PLAYER_MAX_VELY;
		m_friction = WORLD_FRICTION;

		m_vel = { 0.f, 0.f, 0.f };
	}
	const vec3& CPhysic::CalculateMoveShift(char dir, const vec3& look, const vec3& right)
	{
		vec3 shift = { 0, 0, 0 };

		if (dir & DIR_FORWARD) {
			shift = vec3::Add(shift, look, PLAYER_SPEED);
		}
		if (dir & DIR_BACKWARD) {
			shift = vec3::Add(shift, look, -PLAYER_SPEED);
		}
		if (dir & DIR_RIGHT) {
			shift = vec3::Add(shift, right, PLAYER_SPEED);
		}
		if (dir & DIR_LEFT) {
			shift = vec3::Add(shift, right, -PLAYER_SPEED);
		}
		m_vel += shift;

		float velocity = sqrtf(m_vel.x * m_vel.x + m_vel.z * m_vel.z);

		if (velocity > m_maxVelXZ) {
			m_vel.x *= (m_maxVelXZ / velocity);
			m_vel.z *= (m_maxVelXZ / velocity);
		}

		return m_vel;
	}

	void CPhysic::Deceleration(float elapsedTime)
	{
		float deceleration = m_friction * elapsedTime;
		if (deceleration > m_vel.Length())
			deceleration = m_vel.Length();
		m_vel = vec3::Add(m_vel, vec3::Normalize(m_vel * -deceleration));
	}
}