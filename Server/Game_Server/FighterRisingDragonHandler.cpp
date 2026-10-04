#include "pch.h"
#include "FighterRisingDragonHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    

    FighterRisingDragonHandler::FighterRisingDragonHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {        
        m_type = EPlayerSkill::FighterRisingDragon;
        SetSkillInfo();
    }

    CSkillHandler* FighterRisingDragonHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterRisingDragonHandler(client);
    }

    void FighterRisingDragonHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, vec3(1.f, m_client->GetPos().y + skillCsv->posOffset, 0.f), SkillDamage(), 0, SkillUseTime()));
    }

}