#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManAnkleCutHandler : public CAttackSkillHandler
	{
	public:
		SwordManAnkleCutHandler() {}
		SwordManAnkleCutHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;		
	};

}