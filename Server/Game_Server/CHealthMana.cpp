#include "pch.h"
#include "CHealthMana.h"

namespace wod_server {
	bool CHealthMana::Damage(int _damage, int _critical, int _armor, int _regist, DAMAGE_TYPE _type)
	{
		int calculatedDamage = 0;
		if (_type == DAMAGE_TYPE::STRENGTH) {
			calculatedDamage = static_cast<int>(_damage * static_cast<float>((100.f / (100.f + _armor))));
		}
		else {
			calculatedDamage = static_cast<int>(_damage * static_cast<float>((100.f / (100.f + _regist))));
		}

		if (_critical > 0) {
			if (*RandomUtil::GenerateUniqueRandomNumbers(0, 99, 1).begin() < _critical) {
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