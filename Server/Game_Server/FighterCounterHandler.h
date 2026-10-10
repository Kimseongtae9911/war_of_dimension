#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterCounterHandler : public CSkillHandler
	{
	public:
		FighterCounterHandler() {}
		FighterCounterHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}