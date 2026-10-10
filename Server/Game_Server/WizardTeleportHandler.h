#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardTeleportHandler : public CSkillHandler
	{
	public:
		WizardTeleportHandler() {}
		WizardTeleportHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};
}

