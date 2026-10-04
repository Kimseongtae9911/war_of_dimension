#include "pch.h"
#include "ArcherPhoenixArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherPhoenixArrowHandler::ArcherPhoenixArrowHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ArcherPhoenixArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherPhoenixArrowHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherPhoenixArrowHandler(client);
    }
}