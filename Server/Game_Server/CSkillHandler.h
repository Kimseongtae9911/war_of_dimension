#pragma once

#include "CClient.h"
#include "CNpc.h"

namespace wod_server {

	class CSkillHandler : ISkillHandler
	{
	public:
		CSkillHandler() {}
		CSkillHandler(std::shared_ptr<CClient> client) { m_client = client; }
		~CSkillHandler() {}

		virtual CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) = 0;

		virtual void Handle() override {};

		void SetClient(std::shared_ptr<CClient> client) { m_client = client; }

	protected:
		std::chrono::time_point<std::chrono::system_clock> SkillUseTime() { return TimeUtil::PassedTimeMSec(m_castingTime); }
		int SkillDamage();
		vec3 SkillStartPos() { return m_client->GetPos() + m_client->GetLook() * m_startPosOffset; }
		void SetSkillInfo();

	protected:
		std::shared_ptr<CClient> m_client;
		
		int m_castingTime = 0;
		float m_strengthRatio = 0.f;
		float m_magicRatio = 0.f;
		float m_speedRatio = 0.f;
		float m_startPosOffset = 0.f;
		EPlayerSkill m_type = EPlayerSkill::None;
	};

	class CAttackSkillHandler : public CSkillHandler
	{
	public:
		CAttackSkillHandler() {}
		CAttackSkillHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}

		virtual void Handle() override;
	};
}