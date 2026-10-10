#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterAttackHandler : public CSkillHandler
	{
	public:
		FighterAttackHandler() {}
		FighterAttackHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}