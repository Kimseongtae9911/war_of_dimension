#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherVitalPointHandler : public CSkillHandler
	{
	public:
		ArcherVitalPointHandler() {}
		ArcherVitalPointHandler(std::shared_ptr<CClient> client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}