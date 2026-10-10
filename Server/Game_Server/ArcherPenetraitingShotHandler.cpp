#include "pch.h"
#include "ArcherPenetraitingShotHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherPenetraitingShotHandler::ArcherPenetraitingShotHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherPenetraitingShot;
        SetSkillInfo();
    }

    CSkillHandler* ArcherPenetraitingShotHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherPenetraitingShotHandler(_client);
    }
}