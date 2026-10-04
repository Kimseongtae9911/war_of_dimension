#include "pch.h"
#include "OgreRockThrowHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    OgreRockThrowHandler::OgreRockThrowHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::OgreRockThrow;
        SetSkillInfo();
    }

    CSkillHandler* OgreRockThrowHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreRockThrowHandler(client);
    }  
}