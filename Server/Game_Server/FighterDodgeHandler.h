#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterDodgeHandler : public CSkillHandler
	{
	public:
		FighterDodgeHandler() {}
		FighterDodgeHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}