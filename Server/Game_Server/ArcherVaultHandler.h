#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherVaultHandler : public CSkillHandler
	{
	public:
		ArcherVaultHandler() {}
		ArcherVaultHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}