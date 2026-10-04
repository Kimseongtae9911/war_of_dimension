#include "pch.h"
#include "ArcherPenetraitingShotHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherPenetraitingShotHandler::ArcherPenetraitingShotHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {       
        m_type = EPlayerSkill::ArcherPenetraitingShot;
        SetSkillInfo();
    }

    CSkillHandler* ArcherPenetraitingShotHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherPenetraitingShotHandler(client);
    }
}