#include "pch.h"
#include "ProDeleteHandler.h"

namespace wod_server {

    ProDeleteHandler::ProDeleteHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ProgrammerDelete;
        SetSkillInfo();
    }

    CSkillHandler* ProDeleteHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProDeleteHandler(_client);
    }

}