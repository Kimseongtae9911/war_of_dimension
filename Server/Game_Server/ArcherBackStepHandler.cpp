#include "pch.h"
#include "ArcherBackStepHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherBackStepHandler::ArcherBackStepHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherBackStep;
    }

    CSkillHandler* ArcherBackStepHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherBackStepHandler(client);
    }

    void ArcherBackStepHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent({ m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook() * (-1.0f), 0, 0, SkillUseTime() });
    }

}