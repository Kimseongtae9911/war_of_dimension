#include "pch.h"
#include "ProHelloWorldHandler.h"

namespace wod_server {

    ProHelloWorldHandler::ProHelloWorldHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ProgrammerHelloWorld;
        SetSkillInfo();
    }

    CSkillHandler* ProHelloWorldHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProHelloWorldHandler(_client);
    }

}