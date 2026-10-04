#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherMultipleShotHandler : public CAttackSkillHandler
	{
	public:
		ArcherMultipleShotHandler() {}
		ArcherMultipleShotHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}