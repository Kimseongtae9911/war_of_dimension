#include "pch.h"
#include "FighterFireBallHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterFireBallHandler::FighterFireBallHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::FighterFireBall;
        SetSkillInfo();
    }

    CSkillHandler* FighterFireBallHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterFireBallHandler(client);
    }
}