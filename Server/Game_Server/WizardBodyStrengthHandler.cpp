#include "pch.h"
#include "WizardBodyStrengthHandler.h"

namespace wod_server {
    CSkillHandler* WizardBodyStrengthHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardBodyStrengthHandler(_client);
    }

    void WizardBodyStrengthHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardBodyStrength);
        auto strengthBuff = skillCsv->m_buffInfo[EBuffType::StrengthIncrease];
        auto speedBuff = skillCsv->m_buffInfo[EBuffType::SpeedIncrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.m_strength += static_cast<int>(strengthBuff.m_buffValue);
        stat.m_speed += speedBuff.m_buffValue;
        m_client->GetStatus()->SetStat(stat);
        CStat changeStat = CStat(0);
        changeStat.m_strength = static_cast<int>(strengthBuff.m_buffValue);
        changeStat.m_speed = speedBuff.m_buffValue;

        //Skill End Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(strengthBuff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }

}