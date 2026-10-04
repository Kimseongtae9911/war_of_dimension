#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManBerserkHandler : public CSkillHandler
	{
	public:
		SwordManBerserkHandler() {}
		SwordManBerserkHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}