#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManJudgementSwordHandler : public CSkillHandler
	{
	public:
		SwordManJudgementSwordHandler() {}
		SwordManJudgementSwordHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}