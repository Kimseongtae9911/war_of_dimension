#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterIndestructibleHandler : public CSkillHandler
	{
	public:
		FighterIndestructibleHandler() {}
		FighterIndestructibleHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}