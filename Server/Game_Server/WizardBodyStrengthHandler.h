#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardBodyStrengthHandler : public CSkillHandler
	{
	public:
		WizardBodyStrengthHandler() {}
		WizardBodyStrengthHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}