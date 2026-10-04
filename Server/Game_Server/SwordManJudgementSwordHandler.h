#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManJudgementSwordHandler : public CSkillHandler
	{
	public:
		SwordManJudgementSwordHandler() {}
		SwordManJudgementSwordHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}