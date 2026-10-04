#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManDodgeHandler : public CSkillHandler
	{
	public:
		SwordManDodgeHandler() {}
		SwordManDodgeHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}