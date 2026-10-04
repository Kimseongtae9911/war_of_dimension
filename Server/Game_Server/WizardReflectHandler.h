#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardReflectHandler : public CSkillHandler
	{
	public:
		WizardReflectHandler() {}
		WizardReflectHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}