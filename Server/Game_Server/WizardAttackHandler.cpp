#include "pch.h"
#include "WizardAttackHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardAttackHandler::WizardAttackHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardAttack;
        SetSkillInfo();
    }

    CSkillHandler* WizardAttackHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardAttackHandler(_client);
    }
}