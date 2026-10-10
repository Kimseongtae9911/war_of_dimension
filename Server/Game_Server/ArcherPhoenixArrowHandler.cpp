#include "pch.h"
#include "ArcherPhoenixArrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherPhoenixArrowHandler::ArcherPhoenixArrowHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherPhoenixArrow;
        SetSkillInfo();
    }

    CSkillHandler* ArcherPhoenixArrowHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherPhoenixArrowHandler(_client);
    }
}