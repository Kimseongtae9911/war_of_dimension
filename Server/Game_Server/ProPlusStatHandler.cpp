#include "pch.h"
#include "ProPlusStatHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    CSkillHandler* ProPlusStatHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProPlusStatHandler(client);
    }

    void ProPlusStatHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPlusStats);
        auto speedBuff = skillCsv->buffInfo[EBuffType::SpeedIncrease];
        auto statBuff = skillCsv->buffInfo[EBuffType::StatIncrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.armor += static_cast<int>(statBuff.buffValue);
        stat.critical += static_cast<int>(statBuff.buffValue);
        stat.endure += static_cast<int>(statBuff.buffValue);
        stat.magic += static_cast<int>(statBuff.buffValue);
        stat.regist += static_cast<int>(statBuff.buffValue);
        stat.strength += static_cast<int>(statBuff.buffValue);
        stat.speed += speedBuff.buffValue;
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat(0);
        changeStat.armor = static_cast<int>(statBuff.buffValue);
        changeStat.critical = static_cast<int>(statBuff.buffValue);
        changeStat.endure = static_cast<int>(statBuff.buffValue);
        changeStat.magic = static_cast<int>(statBuff.buffValue);
        changeStat.regist = static_cast<int>(statBuff.buffValue);
        changeStat.strength = static_cast<int>(statBuff.buffValue);
        changeStat.speed = speedBuff.buffValue;
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(speedBuff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });

    }

}