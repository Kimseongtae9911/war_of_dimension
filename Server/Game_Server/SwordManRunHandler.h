#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManRunHandler : public CSkillHandler
	{
	public:
		SwordManRunHandler() {}
		SwordManRunHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}