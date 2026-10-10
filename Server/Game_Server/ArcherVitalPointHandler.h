#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ArcherVitalPointHandler : public CSkillHandler
	{
	public:
		ArcherVitalPointHandler() {}
		ArcherVitalPointHandler(std::shared_ptr<CClient> _client);

		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}