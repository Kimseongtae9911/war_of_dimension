#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProSCLHandler : public CAttackSkillHandler
	{
	public:
		ProSCLHandler() {}
		ProSCLHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}