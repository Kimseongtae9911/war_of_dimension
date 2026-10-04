#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterRisingDragonHandler : public CSkillHandler
	{
	public:
		FighterRisingDragonHandler() {}
		FighterRisingDragonHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}