#include "pch.h"
#include "CStat.h"

namespace wod_server {
    void CStat::ChangeStatUntilRollback(int _clientId, const CStat& _changeStat, time_t _rollbackTime)
    {
		m_speed += _changeStat.m_speed;
		m_strength += _changeStat.m_strength;
		m_magic += _changeStat.m_magic;
		m_armor += _changeStat.m_armor;
		m_regist += _changeStat.m_regist;
		m_endure += _changeStat.m_endure;
		m_critical += _changeStat.m_critical;

		network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(_clientId, TimeUtil::PassedTimeMSec(_rollbackTime), EVENT_TYPE::EV_STAT_CHANGE, -1, _changeStat));
    }
}