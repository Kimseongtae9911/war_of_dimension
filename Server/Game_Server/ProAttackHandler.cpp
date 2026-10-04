#include "pch.h"
#include "ProAttackHandler.h"

namespace wod_server {

    ProAttackHandler::ProAttackHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::ProgrammerAttack;
        SetSkillInfo();
    }

    CSkillHandler* ProAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProAttackHandler(client);
    }
}