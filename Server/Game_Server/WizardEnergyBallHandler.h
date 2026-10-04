#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardEnergyBallHandler : public CAttackSkillHandler
	{
	public:
		WizardEnergyBallHandler() {}
		WizardEnergyBallHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}