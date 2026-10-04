#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardBlinkHandler : public CSkillHandler
	{
	public:
		WizardBlinkHandler() {}
		WizardBlinkHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}