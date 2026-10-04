#include "pch.h"
#include "WizardDarknessRayHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardDarknessRayHandler::WizardDarknessRayHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::WizardDarknessRay;
        SetSkillInfo();
    }

    CSkillHandler* WizardDarknessRayHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardDarknessRayHandler(client);
    }
}