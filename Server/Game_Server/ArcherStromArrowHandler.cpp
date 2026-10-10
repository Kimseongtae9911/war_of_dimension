#include "pch.h"
#include "ArcherStromArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherStromArrowHandler::ArcherStromArrowHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherStormArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherStromArrowHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherStromArrowHandler(_client);
    }
}