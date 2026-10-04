#include "pch.h"
#include "WizardAttackHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardAttackHandler::WizardAttackHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::WizardAttack;
        SetSkillInfo();
    }

    CSkillHandler* WizardAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardAttackHandler(client);
    }
}