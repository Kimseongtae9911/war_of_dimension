#include "pch.h"
#include "WizardMagicMissilHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardMagicMissilHandler::WizardMagicMissilHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::WizardMagicMissile;
        SetSkillInfo();
    }

    CSkillHandler* WizardMagicMissilHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardMagicMissilHandler(client);
    }
}