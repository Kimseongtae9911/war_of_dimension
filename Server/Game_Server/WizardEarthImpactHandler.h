#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardEarthImpactHandler : public CAttackSkillHandler
	{
	public:
		WizardEarthImpactHandler() {}
		WizardEarthImpactHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}