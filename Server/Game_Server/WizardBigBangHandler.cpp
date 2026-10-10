#include "pch.h"
#include "WizardBigBangHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

	WizardBigBangHandler::WizardBigBangHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
	{
		m_type = EPlayerSkill::WizardBigBang;
		SetSkillInfo();
	}

	CSkillHandler* WizardBigBangHandler::CreateHandler(std::shared_ptr<CClient> _client)
	{
		return new WizardBigBangHandler(_client);
	}
}
