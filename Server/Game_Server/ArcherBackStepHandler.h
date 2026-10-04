#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherBackStepHandler : public CSkillHandler
	{
	public:
		ArcherBackStepHandler() {}
		ArcherBackStepHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}