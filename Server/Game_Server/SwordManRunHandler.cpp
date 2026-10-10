#include "pch.h"
#include "SwordManRunHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* SwordManRunHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManRunHandler(_client);
    }

    void SwordManRunHandler::Handle()
    {
        auto runBuff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManRun)->m_buffInfo[EBuffType::SpeedIncrease];

        m_client->SetUsingSkill(false);
        CStat stat = m_client->GetStatus()->GetStat();
        stat.m_speed += runBuff.m_buffValue;
        m_client->GetStatus()->SetStat(stat);
        CStat changeStat = CStat(0);
        changeStat.m_speed = runBuff.m_buffValue;

        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(runBuff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }
}
