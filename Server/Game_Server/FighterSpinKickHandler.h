#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterSpinKickHandler : public CSkillHandler
	{
	public:
		FighterSpinKickHandler() {}
		FighterSpinKickHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}