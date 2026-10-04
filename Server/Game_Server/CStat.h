#pragma once

namespace wod_server {
	class CStat
	{
	public:
		CStat() {}
		CStat(int type) { if (type == 0) { strength = magic = armor = regist = endure = critical = 0; speed = 0.f; } }
		CStat(float speed, int strength, int magic, int armor, int regist, int endure, int critical) {
			this->speed = speed; this->strength = strength; this->magic = magic; this->armor = armor; this->regist = regist; this->endure = endure; this->critical = critical;
		}

		void ChangeStatUntilRollback(int _clientId, const CStat& _changeStat, time_t _rollbackTime);

		float speed = 1;
		int strength = 1;
		int magic = 1;
		int armor = 0;
		int regist = 1;
		int endure = 0;
		int critical = 0;

		friend CStat  operator -(const CStat& l, const CStat& r)
		{
			return CStat(l.speed - r.speed, l.strength - r.strength, l.magic - r.magic, l.armor - r.armor, l.regist - r.regist, l.endure - r.endure, l.critical - r.critical);
		}
		friend CStat  operator +(const CStat& l, const CStat& r)
		{
			return CStat(l.speed + r.speed, l.strength + r.strength, l.magic + r.magic, l.armor + r.armor, l.regist + r.regist, l.endure + r.endure, l.critical + r.critical);
		}
	};

}