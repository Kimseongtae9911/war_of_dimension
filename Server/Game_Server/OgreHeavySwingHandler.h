#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreHeavySwingHandler : public CAttackSkillHandler
	{
	public:
		OgreHeavySwingHandler() {}
		OgreHeavySwingHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}