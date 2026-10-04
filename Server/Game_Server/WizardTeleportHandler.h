#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardTeleportHandler : public CSkillHandler
	{
	public:
		WizardTeleportHandler() {}
		WizardTeleportHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};
}

