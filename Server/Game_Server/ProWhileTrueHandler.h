#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProWhileTrueHandler : public CSkillHandler
	{
	public:
		ProWhileTrueHandler() {}
		ProWhileTrueHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}