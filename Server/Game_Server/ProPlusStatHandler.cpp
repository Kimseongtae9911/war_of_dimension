#include "pch.h"
#include "ProPlusStatHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    CSkillHandler* ProPlusStatHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProPlusStatHandler(_client);
    }

    void ProPlusStatHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPlusStats);
        auto speedBuff = skillCsv->m_buffInfo[EBuffType::SpeedIncrease];
        auto statBuff = skillCsv->m_buffInfo[EBuffType::StatIncrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.m_armor += static_cast<int>(statBuff.m_buffValue);
        stat.m_critical += static_cast<int>(statBuff.m_buffValue);
        stat.m_endure += static_cast<int>(statBuff.m_buffValue);
        stat.m_magic += static_cast<int>(statBuff.m_buffValue);
        stat.m_regist += static_cast<int>(statBuff.m_buffValue);
        stat.m_strength += static_cast<int>(statBuff.m_buffValue);
        stat.m_speed += speedBuff.m_buffValue;
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat(0);
        changeStat.m_armor = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_critical = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_endure = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_magic = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_regist = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_strength = static_cast<int>(statBuff.m_buffValue);
        changeStat.m_speed = speedBuff.m_buffValue;
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(speedBuff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });

    }

}