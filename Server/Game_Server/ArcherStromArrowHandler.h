#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherStromArrowHandler : public CAttackSkillHandler
	{
	public:
		ArcherStromArrowHandler() {}
		ArcherStromArrowHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}