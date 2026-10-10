#include "pch.h"
#include "ProAttackHandler.h"

namespace wod_server {

    ProAttackHandler::ProAttackHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ProgrammerAttack;
        SetSkillInfo();
    }

    CSkillHandler* ProAttackHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProAttackHandler(_client);
    }
}