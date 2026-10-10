#include "pch.h"
#include "WizardEarthImpactHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardEarthImpactHandler::WizardEarthImpactHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardEarthImpact;
        SetSkillInfo();
    }

    CSkillHandler* WizardEarthImpactHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardEarthImpactHandler(_client);
    }
}