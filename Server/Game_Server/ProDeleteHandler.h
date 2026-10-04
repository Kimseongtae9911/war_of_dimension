#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProDeleteHandler : public CAttackSkillHandler
	{
	public:
		ProDeleteHandler() {}
		ProDeleteHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}