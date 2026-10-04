#include "pch.h"
#include "OgreButtingHandler.h"
#include "CNetworkMgr.h"
#include "GameUtil.h"

namespace wod_server {
    OgreButtingHandler::OgreButtingHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::OgreButting;
        SetSkillInfo();
    }

    CSkillHandler* OgreButtingHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreButtingHandler(client);
    }
}