#include "pch.h"
#include "OgreDimensionPunchHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    OgreDimensionPunchHandler::OgreDimensionPunchHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::OgreDimensionPunch;
        SetSkillInfo();
    }

    CSkillHandler* OgreDimensionPunchHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreDimensionPunchHandler(client);
    }

}