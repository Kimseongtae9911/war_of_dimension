#include "pch.h"
#include "ArcherAttackHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherAttackHandler::ArcherAttackHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherAttack;
        SetSkillInfo();
    }

    CSkillHandler* ArcherAttackHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherAttackHandler(_client);
    }
}