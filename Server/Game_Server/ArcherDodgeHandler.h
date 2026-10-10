#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherDodgeHandler : public CSkillHandler
	{
	public:
		ArcherDodgeHandler() {}
		ArcherDodgeHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}