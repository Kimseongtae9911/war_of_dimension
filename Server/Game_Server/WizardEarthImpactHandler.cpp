#include "pch.h"
#include "WizardEarthImpactHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardEarthImpactHandler::WizardEarthImpactHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::WizardEarthImpact;
        SetSkillInfo();
    }

    CSkillHandler* WizardEarthImpactHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardEarthImpactHandler(client);
    }
}