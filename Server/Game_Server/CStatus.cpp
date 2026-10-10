#include "pch.h"
#include "CStatus.h"

namespace wod_server {
	CStatus::CStatus()
	{
		m_stats = {};
		m_defensiveBuff = DEFENSIVE_BUFF::NONE;
		m_damageBuff = DAMAGE_BUFF::NONE;
		m_coolTimeBuff = COOLTIME_BUFF::NONE;
	}

	CStatus::~CStatus()
	{

	}
}