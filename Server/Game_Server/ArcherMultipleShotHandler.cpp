#include "pch.h"
#include "ArcherMultipleShotHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherMultipleShotHandler::ArcherMultipleShotHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ArcherMultipleShot;
        SetSkillInfo();
    }

    CSkillHandler* ArcherMultipleShotHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherMultipleShotHandler(client);
    }
}