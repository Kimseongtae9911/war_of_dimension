#include "pch.h"
#include "WizardEnergyBallHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    WizardEnergyBallHandler::WizardEnergyBallHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::WizardEnergyBall;
        SetSkillInfo();
    }

    CSkillHandler* WizardEnergyBallHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardEnergyBallHandler(_client);
    }
}