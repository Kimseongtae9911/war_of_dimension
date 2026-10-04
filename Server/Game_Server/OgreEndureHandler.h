#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreEndureHandler : public CSkillHandler
	{
	public:
		OgreEndureHandler() {}
		OgreEndureHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}