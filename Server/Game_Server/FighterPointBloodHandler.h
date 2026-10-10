#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class FighterPointBloodHandler : public CSkillHandler
	{
	public:
		FighterPointBloodHandler() {}
		FighterPointBloodHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;

	private:
		SkillCsv* m_skillCsv = nullptr;
	};

}