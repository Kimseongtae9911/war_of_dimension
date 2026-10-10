#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardEnergyBallHandler : public CAttackSkillHandler
	{
	public:
		WizardEnergyBallHandler() {}
		WizardEnergyBallHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}