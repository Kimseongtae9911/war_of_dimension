#pragma once

namespace wod_server {
	class CSkill
	{
	public:
		CSkill();
		~CSkill();

		int GetSkillNum() const { return m_skillNum; }
		int GetSkillCoolTime() const { return m_skillCoolTime; }
		const std::chrono::system_clock::time_point& GetLastSkillTime() const { return m_lastSkillTime; }
		int GetMpConsumption() const { return m_mpConsumption; }

		void SetSkillNum(int _skillNum) { m_skillNum = _skillNum; }
		void SetCoolTime(int _coolTime) { m_skillCoolTime = _coolTime; }
		void SetLastSkillTime(const std::chrono::system_clock::time_point& _time) { m_lastSkillTime = _time; }
		void SetMpConsumption(int _mp) { m_mpConsumption = _mp; }

	private:
		int m_skillNum = 0;
		int m_skillCoolTime = 0;
		int m_mpConsumption = 0;
		std::chrono::system_clock::time_point m_lastSkillTime = {};
	};
}