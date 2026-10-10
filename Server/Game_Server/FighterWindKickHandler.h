#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterWindKickHandler : public CSkillHandler
	{
	public:
		FighterWindKickHandler() {}
		FighterWindKickHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}