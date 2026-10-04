#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProPlusStatHandler : public CSkillHandler
	{
	public:
		ProPlusStatHandler() {}
		ProPlusStatHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}