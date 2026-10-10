#include "pch.h"
#include "WizardDarknessRayHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardDarknessRayHandler::WizardDarknessRayHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardDarknessRay;
        SetSkillInfo();
    }

    CSkillHandler* WizardDarknessRayHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardDarknessRayHandler(_client);
    }
}