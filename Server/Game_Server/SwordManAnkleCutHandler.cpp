#include "pch.h"
#include "SwordManAnkleCutHandler.h"
#include "CNetworkMgr.h"
#include "GameUtil.h"

namespace wod_server {

    SwordManAnkleCutHandler::SwordManAnkleCutHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::SwordManAnkleCut;
        SetSkillInfo();
    }

    CSkillHandler* SwordManAnkleCutHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManAnkleCutHandler(client);
    }
}