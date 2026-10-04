#include "pch.h"
#include "ProDeleteHandler.h"

namespace wod_server {

    ProDeleteHandler::ProDeleteHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ProgrammerDelete;
        SetSkillInfo();
    }

    CSkillHandler* ProDeleteHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProDeleteHandler(client);
    }

}