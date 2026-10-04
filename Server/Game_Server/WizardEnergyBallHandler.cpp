#include "pch.h"
#include "WizardEnergyBallHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardEnergyBallHandler::WizardEnergyBallHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::WizardEnergyBall;
        SetSkillInfo();
    }

    CSkillHandler* WizardEnergyBallHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardEnergyBallHandler(client);
    }
}