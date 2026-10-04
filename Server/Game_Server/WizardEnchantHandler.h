#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardEnchantHandler : public CSkillHandler
	{
	public:
		WizardEnchantHandler() {}
		WizardEnchantHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}