#include "pch.h"
#include "OgreHeavySwingHandler.h"

namespace wod_server {

    OgreHeavySwingHandler::OgreHeavySwingHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::OgreHeavySwing;
        SetSkillInfo();
    }

    CSkillHandler* OgreHeavySwingHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreHeavySwingHandler(client);
    }
}