#include "pch.h"
#include "FighterDodgeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterDodgeHandler::FighterDodgeHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::FighterDodge;
    }

    CSkillHandler* FighterDodgeHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterDodgeHandler(client);
    }

    void FighterDodgeHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent({ m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), 0, 0, TimeUtil::CurTime() });
    }

}