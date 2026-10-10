#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterDragonFistHandler : public CSkillHandler
	{
	public:
		FighterDragonFistHandler() {}
		FighterDragonFistHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}