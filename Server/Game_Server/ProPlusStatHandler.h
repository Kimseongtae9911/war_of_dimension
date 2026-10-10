#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProPlusStatHandler : public CSkillHandler
	{
	public:
		ProPlusStatHandler() {}
		ProPlusStatHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}