#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherVaultHandler : public CSkillHandler
	{
	public:
		ArcherVaultHandler() {}
		ArcherVaultHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;		

		void Handle() override;
	};

}