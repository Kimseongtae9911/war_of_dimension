#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherArrowRainHandler : public CAttackSkillHandler
	{
	public:
		ArcherArrowRainHandler() {}
		ArcherArrowRainHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}