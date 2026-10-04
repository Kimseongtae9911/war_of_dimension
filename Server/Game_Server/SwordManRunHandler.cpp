#include "pch.h"
#include "SwordManRunHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* SwordManRunHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManRunHandler(client);
    }

    void SwordManRunHandler::Handle()
    {
        auto runBuff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManRun)->buffInfo[EBuffType::SpeedIncrease];

        m_client->SetUsingSkill(false);
        CStat stat = m_client->GetStatus()->GetStat();
        stat.speed += runBuff.buffValue;
        m_client->GetStatus()->SetStat(stat);
        CStat changeStat = CStat(0);
        changeStat.speed = runBuff.buffValue;

        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(runBuff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }
}
