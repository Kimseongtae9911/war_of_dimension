#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterIndestructibleHandler : public CSkillHandler
	{
	public:
		FighterIndestructibleHandler() {}
		FighterIndestructibleHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}