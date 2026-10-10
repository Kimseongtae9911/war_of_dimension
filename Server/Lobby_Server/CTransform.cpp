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

	void CTransform::Move(const vec3& _shift, float _height)
	{
		m_pos += _shift;
		m_pos.m_y = _height;
	}

	bool CTransform::CheckDistance(const vec3& _pos)
	{
		if (sqrtf(powf(m_pos.m_x - _pos.m_x, 2.f) + powf(m_pos.m_y - _pos.m_y, 2.f) + powf(m_pos.m_z - _pos.m_z, 2.f)) <= VIEW_DISTANCE)
			return true;

		return false;
	}
}