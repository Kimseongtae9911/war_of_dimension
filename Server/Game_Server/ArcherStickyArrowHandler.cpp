#include "pch.h"
#include "ArcherStickyArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherStickyArrowHandler::ArcherStickyArrowHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherStickyArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherStickyArrowHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherStickyArrowHandler(client);
    }
}