#include "pch.h"
#include "FighterWildAttackHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterWildAttackHandler::FighterWildAttackHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {        
        m_type = EPlayerSkill::FighterWildAttack;
        SetSkillInfo();
    }

    CSkillHandler* FighterWildAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterWildAttackHandler(client);
    }

    void FighterWildAttackHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, {}, SkillDamage(), 0, {}));
    }

}