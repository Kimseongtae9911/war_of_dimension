#include "pch.h"
#include "ProReleaseHandler.h"

namespace wod_server {

    ProReleaseHandler::ProReleaseHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ProgrammerRelease;
        SetSkillInfo();
    }

    CSkillHandler* ProReleaseHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProReleaseHandler(_client);
    }

}