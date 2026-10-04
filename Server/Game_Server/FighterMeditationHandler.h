#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterMeditationHandler : public CSkillHandler
	{
	public:
		FighterMeditationHandler() {}
		FighterMeditationHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}