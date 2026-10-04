#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreButtingHandler : public CAttackSkillHandler
	{
	public:
		OgreButtingHandler() {}
		OgreButtingHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}