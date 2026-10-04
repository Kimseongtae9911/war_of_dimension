#include "pch.h"
#include "FighterWindKickHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterWindKickHandler::FighterWindKickHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {        
        m_type = EPlayerSkill::FighterWindKick;
        SetSkillInfo();
    }

    CSkillHandler* FighterWindKickHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterWindKickHandler(client);
    }

    void FighterWindKickHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), SkillDamage(), 0, SkillUseTime()));
    }

}