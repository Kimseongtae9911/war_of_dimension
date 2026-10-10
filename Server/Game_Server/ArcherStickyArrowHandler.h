#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherStickyArrowHandler : public CAttackSkillHandler
	{
	public:
		ArcherStickyArrowHandler() {}
		ArcherStickyArrowHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}