#pragma once

namespace wod_server {
	struct SKILL_EVENT {
		SKILL_EVENT() { m_objID = 0; m_wakeUpTime = {}; m_skillType = {}; m_pos = {}; m_power = 0; m_repeatTime = 0; m_lastProcessTime = {}; }
		SKILL_EVENT(int _objID, std::chrono::system_clock::time_point _wakeUpTime, EPlayerSkill _skillType, vec3 _pos, int _power, int _repeatTime, std::chrono::system_clock::time_point _lastProcessTime) :
			m_objID(_objID), m_wakeUpTime(_wakeUpTime), m_skillType(_skillType), m_pos(_pos), m_power(_power), m_repeatTime(_repeatTime), m_lastProcessTime(_lastProcessTime) {}
		SKILL_EVENT(int _objID, std::chrono::system_clock::time_point _wakeUpTime, EPlayerSkill _skillType, vec3 _pos, int _power, int _repeatTime, std::chrono::system_clock::time_point _lastProcessTime, int _clientID) :
			m_objID(_objID), m_wakeUpTime(_wakeUpTime), m_skillType(_skillType), m_pos(_pos), m_power(_power), m_repeatTime(_repeatTime), m_lastProcessTime(_lastProcessTime), m_clientID(_clientID) {}


		int m_objID;
		std::chrono::system_clock::time_point m_wakeUpTime;
		EPlayerSkill m_skillType;
		vec3 m_pos;
		int m_power;
		int m_repeatTime;
		std::chrono::system_clock::time_point m_lastProcessTime;
		int m_clientID;

		constexpr bool operator < (const SKILL_EVENT& _l) const
		{
			return (m_wakeUpTime > _l.m_wakeUpTime);
		}
	};
}