#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManShieldBashHandler : public CSkillHandler
	{
	public:
		SwordManShieldBashHandler() {}
		SwordManShieldBashHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}