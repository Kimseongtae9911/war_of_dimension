#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProSCMHandler : public CSkillHandler
	{
	public:
		ProSCMHandler() {}
		ProSCMHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}