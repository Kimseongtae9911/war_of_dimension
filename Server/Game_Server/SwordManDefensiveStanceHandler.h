#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManDefensiveStanceHandler : public CSkillHandler
	{
	public:
		SwordManDefensiveStanceHandler() {}
		SwordManDefensiveStanceHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}