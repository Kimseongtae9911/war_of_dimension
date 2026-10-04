#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherDodgeHandler : public CSkillHandler
	{
	public:
		ArcherDodgeHandler() {}
		ArcherDodgeHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}