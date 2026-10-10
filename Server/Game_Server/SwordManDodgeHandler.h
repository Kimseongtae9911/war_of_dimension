#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManDodgeHandler : public CSkillHandler
	{
	public:
		SwordManDodgeHandler() {}
		SwordManDodgeHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}