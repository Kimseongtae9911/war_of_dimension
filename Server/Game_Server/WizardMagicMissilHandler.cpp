#include "pch.h"
#include "WizardMagicMissilHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardMagicMissilHandler::WizardMagicMissilHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardMagicMissile;
        SetSkillInfo();
    }

    CSkillHandler* WizardMagicMissilHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardMagicMissilHandler(_client);
    }
}