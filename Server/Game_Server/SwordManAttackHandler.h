#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManAttackHandler : public CSkillHandler
	{
	public:
		SwordManAttackHandler() {}
		SwordManAttackHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}