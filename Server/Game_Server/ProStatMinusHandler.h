#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProStatMinusHandler : public CSkillHandler
	{
	public:
		ProStatMinusHandler() {}
		ProStatMinusHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}