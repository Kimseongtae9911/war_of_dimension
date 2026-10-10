#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProStatMinusHandler : public CSkillHandler
	{
	public:
		ProStatMinusHandler() {}
		ProStatMinusHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}