#pragma once
#include "CHealthMana.h"

namespace wod_server {
	class CStatus
	{
	public:
		CStatus();
		~CStatus();

		CStat GetStat() { statLock.lock(); CStat stat = m_stats; statLock.unlock(); return stat; }
		void SetStat(const CStat& stat) { statLock.lock();  m_stats = stat; statLock.unlock(); }

		CHealthMana healthMana;
		DEFENSIVE_BUFF defensiveBuff = DEFENSIVE_BUFF::NONE;
		DAMAGE_BUFF damageBuff = DAMAGE_BUFF::NONE;
		COOLTIME_BUFF coolTimeBuff = COOLTIME_BUFF::NONE;
		SKILL_BUFF skillBuff = SKILL_BUFF::NONE;

		std::mutex statLock;

	private:
		CStat m_stats;
	};
}