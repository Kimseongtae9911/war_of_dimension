#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManHeavySlashHandler : public CAttackSkillHandler
	{
	public:
		SwordManHeavySlashHandler() {}
		SwordManHeavySlashHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}