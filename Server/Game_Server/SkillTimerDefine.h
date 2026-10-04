#pragma once

namespace wod_server {
	struct SKILL_EVENT {
		SKILL_EVENT() { objID = 0; wakeUpTime = {}; skillType = {}; pos = {}; power = 0; repeatTime = 0; lastProcessTime = {}; }
		SKILL_EVENT(int objID, std::chrono::system_clock::time_point wakeUpTime, EPlayerSkill skillType, vec3 pos, int power, int repeatTime, std::chrono::system_clock::time_point lastProcessTime) :
			objID(objID), wakeUpTime(wakeUpTime), skillType(skillType), pos(pos), power(power), repeatTime(repeatTime), lastProcessTime(lastProcessTime) {}
		SKILL_EVENT(int objID, std::chrono::system_clock::time_point wakeUpTime, EPlayerSkill skillType, vec3 pos, int power, int repeatTime, std::chrono::system_clock::time_point lastProcessTime, int clientID) :
			objID(objID), wakeUpTime(wakeUpTime), skillType(skillType), pos(pos), power(power), repeatTime(repeatTime), lastProcessTime(lastProcessTime), clientID(clientID) {}


		int objID;
		std::chrono::system_clock::time_point wakeUpTime;
		EPlayerSkill skillType;
		vec3 pos;
		int power;
		int repeatTime;
		std::chrono::system_clock::time_point lastProcessTime;
		int clientID;

		constexpr bool operator < (const SKILL_EVENT& L) const
		{
			return (wakeUpTime > L.wakeUpTime);
		}
	};
}