#include "pch.h"
#include "ProSCLHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    ProSCLHandler::ProSCLHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ProgrammerLaser;
        SetSkillInfo();
    }

    CSkillHandler* ProSCLHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProSCLHandler(_client);
    }

}