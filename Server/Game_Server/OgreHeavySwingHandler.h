#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreHeavySwingHandler : public CAttackSkillHandler
	{
	public:
		OgreHeavySwingHandler() {}
		OgreHeavySwingHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}