#include "pch.h"
#include "OgreAttackHandler.h"

namespace wod_server {
    OgreAttackHandler::OgreAttackHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::OgreAttack;
        SetSkillInfo();
    }

    CSkillHandler* OgreAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreAttackHandler(client);
    }
}