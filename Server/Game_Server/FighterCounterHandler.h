#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterCounterHandler : public CSkillHandler
	{
	public:
		FighterCounterHandler() {}
		FighterCounterHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}