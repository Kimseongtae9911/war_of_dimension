#include "pch.h"
#include "WizardBodyStrengthHandler.h"

namespace wod_server {
    CSkillHandler* WizardBodyStrengthHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardBodyStrengthHandler(client);
    }

    void WizardBodyStrengthHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardBodyStrength);
        auto strengthBuff = skillCsv->buffInfo[EBuffType::StrengthIncrease];
        auto speedBuff = skillCsv->buffInfo[EBuffType::SpeedIncrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.strength += static_cast<int>(strengthBuff.buffValue);
        stat.speed += speedBuff.buffValue;
        m_client->GetStatus()->SetStat(stat);
        CStat changeStat = CStat(0);
        changeStat.strength = static_cast<int>(strengthBuff.buffValue);
        changeStat.speed = speedBuff.buffValue;

        //Skill End Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(strengthBuff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }

}