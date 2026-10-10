#include "pch.h"
#include "FighterRisingDragonHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {


    FighterRisingDragonHandler::FighterRisingDragonHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterRisingDragon;
        SetSkillInfo();
    }

    CSkillHandler* FighterRisingDragonHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterRisingDragonHandler(_client);
    }

    void FighterRisingDragonHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, vec3(1.f, m_client->GetPos().m_y + skillCsv->m_posOffset, 0.f), SkillDamage(), 0, SkillUseTime()));
    }

}