#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterDragonFistHandler : public CSkillHandler
	{
	public:
		FighterDragonFistHandler() {}
		FighterDragonFistHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}