#include "pch.h"
#include "ArcherStromArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherStromArrowHandler::ArcherStromArrowHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ArcherStormArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherStromArrowHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherStromArrowHandler(client);
    }
}