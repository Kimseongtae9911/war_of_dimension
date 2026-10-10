#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreDimensionPunchHandler : public CAttackSkillHandler
	{
	public:
		OgreDimensionPunchHandler() {}
		OgreDimensionPunchHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}