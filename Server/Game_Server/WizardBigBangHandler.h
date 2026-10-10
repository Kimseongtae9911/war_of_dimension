#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardBigBangHandler : public CAttackSkillHandler
	{
	public:
		WizardBigBangHandler() {}
		WizardBigBangHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}