#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardDarknessRayHandler : public CAttackSkillHandler
	{
	public:
		WizardDarknessRayHandler() {}
		WizardDarknessRayHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}