#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardBigBangHandler : public CAttackSkillHandler
	{
	public:
		WizardBigBangHandler() {}
		WizardBigBangHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}