#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardMagicMissilHandler : public CAttackSkillHandler
	{
	public:
		WizardMagicMissilHandler() {}
		WizardMagicMissilHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;		
	};

}