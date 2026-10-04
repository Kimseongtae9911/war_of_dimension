#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreAttackHandler : public CAttackSkillHandler
	{
	public:
		OgreAttackHandler() {}
		OgreAttackHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}