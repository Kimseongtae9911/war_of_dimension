#include "pch.h"
#include "WizardMagicEyeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardMagicEyeHandler::WizardMagicEyeHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::WizardMagicEye;
        SetSkillInfo();
    }

    CSkillHandler* WizardMagicEyeHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardMagicEyeHandler(client);
    }
}