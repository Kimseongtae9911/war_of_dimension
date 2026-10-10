#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManRunHandler : public CSkillHandler
	{
	public:
		SwordManRunHandler() {}
		SwordManRunHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}