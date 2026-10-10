#pragma once

namespace wod_server {
	class CHealthMana
	{
	public:
		CHealthMana() {}

		int GetMaxHp() { return m_maxHp; }
		int GetCurHp() { m_hpLock.lock();  int temp = m_curHp; m_hpLock.unlock(); return temp; }
		int GetMaxMp() { return m_maxMp; }
		int GetCurMp() { m_mpLock.lock();  int temp = m_curMp; m_mpLock.unlock(); return temp; }

		void SetMaxHp(int _maxHp) { m_maxHp = _maxHp; }
		void SetCurHp(int _curHp) { m_hpLock.lock(); m_curHp = _curHp; m_hpLock.unlock(); }
		void SetMaxMp(int _maxMp) { m_maxMp = _maxMp; }
		void SetCurMp(int _curMp) { m_mpLock.lock(); m_curMp = _curMp; m_mpLock.unlock(); }

		bool Damage(int _damage, int _critical, int _armor, int _regist, DAMAGE_TYPE _type);

		void UseMp(int _useMp) {
			m_mpLock.lock();
			m_curMp -= _useMp;
			if (m_curMp < 0) {
				m_curMp = 0;
			}
			m_mpLock.unlock();
		}

		void HealHp(int _heal) {
			m_hpLock.lock();
			m_curHp += _heal;
			if (m_curHp > m_maxHp) {
				m_curHp = m_maxHp;
			}
			m_hpLock.unlock();
		}

		void HealMp(int _heal) {
			m_mpLock.lock();
			m_curMp += _heal;
			if (m_curMp > m_maxMp) {
				m_curMp = m_maxMp;
			}
			m_mpLock.unlock();
		}

	private:
		int m_maxHp;
		int m_maxMp;
		int m_curHp;
		int m_curMp;
		std::mutex m_hpLock;
		std::mutex m_mpLock;
	};
}