#pragma once

namespace wod_server {
	class CStat
	{
	public:
		CStat() {}
		CStat(int _type) { if (_type == 0) { m_strength = m_magic = m_armor = m_regist = m_endure = m_critical = 0; m_speed = 0.f; } }
		CStat(float _speed, int _strength, int _magic, int _armor, int _regist, int _endure, int _critical) {
			this->m_speed = _speed; this->m_strength = _strength; this->m_magic = _magic; this->m_armor = _armor; this->m_regist = _regist; this->m_endure = _endure; this->m_critical = _critical;
		}

		void ChangeStatUntilRollback(int _clientId, const CStat& _changeStat, time_t _rollbackTime);

		float m_speed = 1;
		int m_strength = 1;
		int m_magic = 1;
		int m_armor = 0;
		int m_regist = 1;
		int m_endure = 0;
		int m_critical = 0;

		friend CStat  operator -(const CStat& _l, const CStat& _r)
		{
			return CStat(_l.m_speed - _r.m_speed, _l.m_strength - _r.m_strength, _l.m_magic - _r.m_magic, _l.m_armor - _r.m_armor, _l.m_regist - _r.m_regist, _l.m_endure - _r.m_endure, _l.m_critical - _r.m_critical);
		}
		friend CStat  operator +(const CStat& _l, const CStat& _r)
		{
			return CStat(_l.m_speed + _r.m_speed, _l.m_strength + _r.m_strength, _l.m_magic + _r.m_magic, _l.m_armor + _r.m_armor, _l.m_regist + _r.m_regist, _l.m_endure + _r.m_endure, _l.m_critical + _r.m_critical);
		}
	};

}