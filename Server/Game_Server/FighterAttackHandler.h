#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterAttackHandler : public CSkillHandler
	{
	public:
		FighterAttackHandler() {}
		FighterAttackHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}