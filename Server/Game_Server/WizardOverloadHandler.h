#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardOverloadHandler : public CSkillHandler
	{
	public:
		WizardOverloadHandler() {}
		WizardOverloadHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}