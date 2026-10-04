#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherWindStepHandler : public CSkillHandler
	{
	public:
		ArcherWindStepHandler() {}
		ArcherWindStepHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}