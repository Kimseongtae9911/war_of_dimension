#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManHeavySlashHandler : public CAttackSkillHandler
	{
	public:
		SwordManHeavySlashHandler() {}
		SwordManHeavySlashHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}