#include "pch.h"
#include "OgreDimensionCrushHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    OgreDimensionCrushHandler::OgreDimensionCrushHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::OgreDimensionCrush;
        SetSkillInfo();
    }

    CSkillHandler* OgreDimensionCrushHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreDimensionCrushHandler(client);
    }
}