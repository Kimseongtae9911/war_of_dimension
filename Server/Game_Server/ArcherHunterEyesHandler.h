#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherHunterEyesHandler : public CSkillHandler
	{
	public:
		ArcherHunterEyesHandler() {}
		ArcherHunterEyesHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}