#include "pch.h"
#include "SwordManShieldBashHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    SwordManShieldBashHandler::SwordManShieldBashHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {        
        m_type = EPlayerSkill::SwordManShieldBash;
        SetSkillInfo();
    }

    CSkillHandler* SwordManShieldBashHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManShieldBashHandler(client);
    }

    void SwordManShieldBashHandler::Handle()
    {       
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), SkillDamage(), 0, SkillUseTime()));
    }
}
