#include "pch.h"
#include "ArcherDodgeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    ArcherDodgeHandler::ArcherDodgeHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherDodge;
    }

    CSkillHandler* ArcherDodgeHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherDodgeHandler(_client);
    }

    void ArcherDodgeHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent({ m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), 0, 0, SkillUseTime() });
    }

}