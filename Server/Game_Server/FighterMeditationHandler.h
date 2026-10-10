#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterMeditationHandler : public CSkillHandler
	{
	public:
		FighterMeditationHandler() {}
		FighterMeditationHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}