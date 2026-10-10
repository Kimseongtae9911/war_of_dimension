#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManAnkleCutHandler : public CAttackSkillHandler
	{
	public:
		SwordManAnkleCutHandler() {}
		SwordManAnkleCutHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}