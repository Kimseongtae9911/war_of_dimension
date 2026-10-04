#include "pch.h"
#include "ProReleaseHandler.h"

namespace wod_server {

    ProReleaseHandler::ProReleaseHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ProgrammerRelease;
        SetSkillInfo();
    }

    CSkillHandler* ProReleaseHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProReleaseHandler(client);
    }

}