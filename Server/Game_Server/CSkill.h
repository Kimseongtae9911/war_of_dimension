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

		void SetSkillNum(int skillNum) { m_skillNum = skillNum; }
		void SetCoolTime(int coolTime) { m_skillCoolTime = coolTime; }
		void SetLastSkillTime(const std::chrono::system_clock::time_point& time) { m_lastSkillTime = time; }
		void SetMpConsumption(int mp) { m_mpConsumption = mp; }

	private:
		int m_skillNum = 0;
		int m_skillCoolTime = 0;
		int m_mpConsumption = 0;
		std::chrono::system_clock::time_point m_lastSkillTime = {};
	};
}