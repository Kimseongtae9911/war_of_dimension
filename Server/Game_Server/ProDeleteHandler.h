#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProDeleteHandler : public CAttackSkillHandler
	{
	public:
		ProDeleteHandler() {}
		ProDeleteHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}