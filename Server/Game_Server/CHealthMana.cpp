#include "pch.h"
#include "CHealthMana.h"

namespace wod_server {
	bool CHealthMana::Damage(int damage, int critical, int armor, int regist, DAMAGE_TYPE type)
	{
		int calculatedDamage = 0;
		if (type == DAMAGE_TYPE::STRENGTH) {
			calculatedDamage = static_cast<int>(damage * static_cast<float>((100.f / (100.f + armor))));
		}
		else {
			calculatedDamage = static_cast<int>(damage * static_cast<float>((100.f / (100.f + regist))));
		}

		if (critical > 0) {
			if (*RandomUtil::GenerateUniqueRandomNumbers(0, 99, 1).begin() < critical) {
				calculatedDamage = static_cast<int>(calculatedDamage * 1.5f);
			}
		}

		m_hpLock.lock();
		m_curHp -= calculatedDamage;

		if (m_curHp < 0) {
			m_curHp = 0;
			m_hpLock.unlock();
			return true;
		}
		m_hpLock.unlock();
		return false;
	}

}