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

		void SetMaxHp(int maxHp) { m_maxHp = maxHp; }
		void SetCurHp(int curHp) { m_hpLock.lock(); m_curHp = curHp; m_hpLock.unlock(); }
		void SetMaxMp(int maxMp) { m_maxMp = maxMp; }
		void SetCurMp(int curMp) { m_mpLock.lock(); m_curMp = curMp; m_mpLock.unlock(); }

		bool Damage(int damage, int critical, int armor, int regist, DAMAGE_TYPE type);

		void UseMp(int useMp) {
			m_mpLock.lock();
			m_curMp -= useMp;
			if (m_curMp < 0) {
				m_curMp = 0;
			}
			m_mpLock.unlock();
		}

		void HealHp(int heal) {
			m_hpLock.lock();
			m_curHp += heal;
			if (m_curHp > m_maxHp) {
				m_curHp = m_maxHp;
			}
			m_hpLock.unlock();
		}

		void HealMp(int heal) {
			m_mpLock.lock();
			m_curMp += heal;
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