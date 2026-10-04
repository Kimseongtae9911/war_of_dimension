#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterDashHandler : public CSkillHandler
	{
	public:
		FighterDashHandler() {}
		FighterDashHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}