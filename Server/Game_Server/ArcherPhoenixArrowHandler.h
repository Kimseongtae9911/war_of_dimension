#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherPhoenixArrowHandler : public CAttackSkillHandler
	{
	public:
		ArcherPhoenixArrowHandler() {}
		ArcherPhoenixArrowHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}