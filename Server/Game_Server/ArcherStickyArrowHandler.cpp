#include "pch.h"
#include "ArcherStickyArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherStickyArrowHandler::ArcherStickyArrowHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherStickyArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherStickyArrowHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherStickyArrowHandler(_client);
    }
}