#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterWindKickHandler : public CSkillHandler
	{
	public:
		FighterWindKickHandler() {}
		FighterWindKickHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}