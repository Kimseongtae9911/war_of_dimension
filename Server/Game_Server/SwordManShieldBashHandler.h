#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManShieldBashHandler : public CSkillHandler
	{
	public:
		SwordManShieldBashHandler() {}
		SwordManShieldBashHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}