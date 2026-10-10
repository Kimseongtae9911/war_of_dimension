#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManProtectedAreaHandler : public CSkillHandler
	{
	public:
		SwordManProtectedAreaHandler() {}
		SwordManProtectedAreaHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;

	private:
		float m_armorRatio = 0.f;
	};

}