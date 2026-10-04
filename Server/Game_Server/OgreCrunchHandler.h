#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreCrunchHandler : public CSkillHandler
	{
	public:
		OgreCrunchHandler() {}
		OgreCrunchHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}