#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProHelloWorldHandler : public CAttackSkillHandler
	{
	public:
		ProHelloWorldHandler() {}
		ProHelloWorldHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}