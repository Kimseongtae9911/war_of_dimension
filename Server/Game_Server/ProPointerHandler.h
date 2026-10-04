#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProPointerHandler : public CSkillHandler
	{
	public:
		ProPointerHandler() {}
		ProPointerHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}