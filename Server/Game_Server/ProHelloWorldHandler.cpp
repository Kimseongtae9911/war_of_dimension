#include "pch.h"
#include "ProHelloWorldHandler.h"

namespace wod_server {

    ProHelloWorldHandler::ProHelloWorldHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {        
        m_type = EPlayerSkill::ProgrammerHelloWorld;
        SetSkillInfo();
    }

    CSkillHandler* ProHelloWorldHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProHelloWorldHandler(client);
    }

}