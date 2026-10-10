#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherBackStepHandler : public CSkillHandler
	{
	public:
		ArcherBackStepHandler() {}
		ArcherBackStepHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}