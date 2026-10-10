#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProReleaseHandler : public CAttackSkillHandler
	{
	public:
		ProReleaseHandler() {}
		ProReleaseHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}