#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherAttackHandler : public CAttackSkillHandler
	{
	public:
		ArcherAttackHandler() {}
		ArcherAttackHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}