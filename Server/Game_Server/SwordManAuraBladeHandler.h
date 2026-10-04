#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManAuraBladeHandler : public CAttackSkillHandler
	{
	public:
		SwordManAuraBladeHandler() {}
		SwordManAuraBladeHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}