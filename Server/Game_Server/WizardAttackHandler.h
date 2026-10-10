#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardAttackHandler : public CAttackSkillHandler
	{
	public:
		WizardAttackHandler() {}
		WizardAttackHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}