#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreRockThrowHandler : public CAttackSkillHandler
	{
	public:
		OgreRockThrowHandler() {}
		OgreRockThrowHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}