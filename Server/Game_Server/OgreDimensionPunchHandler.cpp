#include "pch.h"
#include "OgreDimensionPunchHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    OgreDimensionPunchHandler::OgreDimensionPunchHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::OgreDimensionPunch;
        SetSkillInfo();
    }

    CSkillHandler* OgreDimensionPunchHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreDimensionPunchHandler(_client);
    }

}