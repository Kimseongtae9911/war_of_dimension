#pragma once
#include "CHealthMana.h"

namespace wod_server {
	class CStatus
	{
	public:
		CStatus();
		~CStatus();

		CStat GetStat() { m_statLock.lock(); CStat stat = m_stats; m_statLock.unlock(); return stat; }
		void SetStat(const CStat& _statValue) { m_statLock.lock();  m_stats = _statValue; m_statLock.unlock(); }

		CHealthMana m_healthMana;
		DEFENSIVE_BUFF m_defensiveBuff = DEFENSIVE_BUFF::NONE;
		DAMAGE_BUFF m_damageBuff = DAMAGE_BUFF::NONE;
		COOLTIME_BUFF m_coolTimeBuff = COOLTIME_BUFF::NONE;
		SKILL_BUFF m_skillBuff = SKILL_BUFF::NONE;

		std::mutex m_statLock;

	private:
		CStat m_stats;
	};
}