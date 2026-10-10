#include "pch.h"
#include "OgreAttackHandler.h"

namespace wod_server {
    OgreAttackHandler::OgreAttackHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::OgreAttack;
        SetSkillInfo();
    }

    CSkillHandler* OgreAttackHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreAttackHandler(_client);
    }
}