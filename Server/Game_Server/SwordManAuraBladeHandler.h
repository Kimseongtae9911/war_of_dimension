#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManAuraBladeHandler : public CAttackSkillHandler
	{
	public:
		SwordManAuraBladeHandler() {}
		SwordManAuraBladeHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}