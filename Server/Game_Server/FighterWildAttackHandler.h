#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterWildAttackHandler : public CSkillHandler
	{
	public:
		FighterWildAttackHandler() {}
		FighterWildAttackHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}