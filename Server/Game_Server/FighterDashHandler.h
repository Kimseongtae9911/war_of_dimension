#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterDashHandler : public CSkillHandler
	{
	public:
		FighterDashHandler() {}
		FighterDashHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}