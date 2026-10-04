#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class ProReturnZeroHandler : public CSkillHandler
	{
	public:
		ProReturnZeroHandler() {}
		ProReturnZeroHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}