#include "pch.h"
#include "ProSCLHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    ProSCLHandler::ProSCLHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::ProgrammerLaser;
        SetSkillInfo();
    }

    CSkillHandler* ProSCLHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProSCLHandler(client);
    }

}