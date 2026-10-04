#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardBodyStrengthHandler : public CSkillHandler
	{
	public:
		WizardBodyStrengthHandler() {}
		WizardBodyStrengthHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}