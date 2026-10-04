#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManHellBladeHandler : public CSkillHandler
	{
	public:
		SwordManHellBladeHandler() {}
		SwordManHellBladeHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}