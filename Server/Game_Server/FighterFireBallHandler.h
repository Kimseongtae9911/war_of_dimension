#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterFireBallHandler : public CAttackSkillHandler
	{
	public:
		FighterFireBallHandler() {}
		FighterFireBallHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}