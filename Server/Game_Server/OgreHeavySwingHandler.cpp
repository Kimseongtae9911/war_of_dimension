#include "pch.h"
#include "OgreHeavySwingHandler.h"

namespace wod_server {

    OgreHeavySwingHandler::OgreHeavySwingHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::OgreHeavySwing;
        SetSkillInfo();
    }

    CSkillHandler* OgreHeavySwingHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreHeavySwingHandler(_client);
    }
}