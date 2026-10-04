#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class SwordManProtectedAreaHandler : public CSkillHandler
	{
	public:
		SwordManProtectedAreaHandler() {}
		SwordManProtectedAreaHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;

	private:
		float m_armorRatio = 0.f;
	};

}