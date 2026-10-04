#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherPenetraitingShotHandler : public CAttackSkillHandler
	{
	public:
		ArcherPenetraitingShotHandler() {}
		ArcherPenetraitingShotHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}