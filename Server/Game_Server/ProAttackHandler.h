#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProAttackHandler : public CAttackSkillHandler
	{
	public:
		ProAttackHandler() {}
		ProAttackHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}