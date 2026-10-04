#include "pch.h"
#include "CStat.h"

namespace wod_server {
    void CStat::ChangeStatUntilRollback(int _clientId, const CStat& _changeStat, time_t _rollbackTime)
    {
		speed += _changeStat.speed;
		strength += _changeStat.strength;
		magic += _changeStat.magic;
		armor += _changeStat.armor;
		regist += _changeStat.regist;
		endure += _changeStat.endure;
		critical += _changeStat.critical;

		network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(_clientId, TimeUtil::PassedTimeMSec(_rollbackTime), EVENT_TYPE::EV_STAT_CHANGE, -1, _changeStat));
    }
}