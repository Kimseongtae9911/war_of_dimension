#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class WizardDarknessRayHandler : public CAttackSkillHandler
	{
	public:
		WizardDarknessRayHandler() {}
		WizardDarknessRayHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}