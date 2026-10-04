#include "pch.h"
#include "CTransform.h"

namespace wod_server {
	CTransform::CTransform()
	{
		m_look = { 0.f, 0.f, 1.f };
		m_right = { 1.f, 0.f, 0.f };
		m_up = { 0.f, 1.f, 0.f };
		m_pos = { 20.0f, 2.6f, 20.0f };
	}

	void CTransform::Reset()
	{
		m_look = { 0.f, 0.f, 1.f };
		m_right = { 1.f, 0.f, 0.f };
		m_up = { 0.f, 1.f, 0.f };
		m_pos = { 20.0f, 2.6f, 20.0f };
	}

	void CTransform::Move(const vec3& shift, float height)
	{
		m_pos += shift;
		m_pos.y = height;
	}

	bool CTransform::CheckDistance(const vec3& pos)
	{
		if (sqrtf(powf(m_pos.x - pos.x, 2.f) + powf(m_pos.y - pos.y, 2.f) + powf(m_pos.z - pos.z, 2.f)) <= VIEW_DISTANCE)
			return true;

		return false;
	}
}