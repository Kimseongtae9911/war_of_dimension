#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreRoarHandler : public CSkillHandler
	{
	public:
		OgreRoarHandler() {}
		OgreRoarHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}