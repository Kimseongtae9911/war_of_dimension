#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherPhoenixArrowHandler : public CAttackSkillHandler
	{
	public:
		ArcherPhoenixArrowHandler() {}
		ArcherPhoenixArrowHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}