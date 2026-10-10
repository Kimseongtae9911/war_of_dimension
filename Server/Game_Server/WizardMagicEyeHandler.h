#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardMagicEyeHandler : public CAttackSkillHandler
	{
	public:
		WizardMagicEyeHandler() {}
		WizardMagicEyeHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}