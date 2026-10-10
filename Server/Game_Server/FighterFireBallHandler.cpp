#include "pch.h"
#include "FighterFireBallHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterFireBallHandler::FighterFireBallHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterFireBall;
        SetSkillInfo();
    }

    CSkillHandler* FighterFireBallHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterFireBallHandler(_client);
    }
}