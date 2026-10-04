#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardMagicEyeHandler : public CAttackSkillHandler
	{
	public:
		WizardMagicEyeHandler() {}
		WizardMagicEyeHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}