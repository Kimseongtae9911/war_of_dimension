#include "pch.h"
#include "SwordManDodgeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* SwordManDodgeHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManDodgeHandler(_client);
    }

    void SwordManDodgeHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent({ m_client->GetID(), TimeUtil::CurTime(), EPlayerSkill::SwordManDodge, m_client->GetLook(), 0, 0, TimeUtil::CurTime()});
    }

}