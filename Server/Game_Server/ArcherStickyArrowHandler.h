#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherStickyArrowHandler : public CAttackSkillHandler
	{
	public:
		ArcherStickyArrowHandler() {}
		ArcherStickyArrowHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;		
	};

}