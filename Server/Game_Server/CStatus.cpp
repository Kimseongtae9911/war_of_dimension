#include "pch.h"
#include "CStatus.h"

namespace wod_server {
	CStatus::CStatus() 
	{
		m_stats = {};
		defensiveBuff = DEFENSIVE_BUFF::NONE;
		damageBuff = DAMAGE_BUFF::NONE;
		coolTimeBuff = COOLTIME_BUFF::NONE;
	}
	
	CStatus::~CStatus()
	{

	}
}