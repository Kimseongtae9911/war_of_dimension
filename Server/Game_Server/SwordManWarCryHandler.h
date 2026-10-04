#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManWarCryHandler : public CSkillHandler
	{
	public:
		SwordManWarCryHandler() {}
		SwordManWarCryHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}