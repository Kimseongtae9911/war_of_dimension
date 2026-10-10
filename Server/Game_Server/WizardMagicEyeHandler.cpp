#include "pch.h"
#include "WizardMagicEyeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardMagicEyeHandler::WizardMagicEyeHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardMagicEye;
        SetSkillInfo();
    }

    CSkillHandler* WizardMagicEyeHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardMagicEyeHandler(_client);
    }
}