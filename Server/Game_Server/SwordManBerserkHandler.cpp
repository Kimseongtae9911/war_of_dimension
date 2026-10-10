#include "pch.h"
#include "SwordManBerserkHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    CSkillHandler* SwordManBerserkHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManBerserkHandler(_client);
    }

    void SwordManBerserkHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManBerserk);
        auto attackBuff = skillCsv->m_buffInfo[EBuffType::AttackIncrease];
        auto utilBuff = skillCsv->m_buffInfo[EBuffType::UtilIncrease];
        auto speedBuff = skillCsv->m_buffInfo[EBuffType::SpeedIncrease];

        auto armorDebuff = skillCsv->m_debuffInfo[EDebuffType::ArmorDecrease];
        auto utilDebuff = skillCsv->m_debuffInfo[EDebuffType::UtilDecrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.m_strength   += static_cast<int>(attackBuff.m_buffValue);
        stat.m_magic      += static_cast<int>(attackBuff.m_buffValue);
        stat.m_critical   += static_cast<int>(utilBuff.m_buffValue);
        stat.m_speed      += speedBuff.m_buffValue;
        stat.m_endure     -= static_cast<int>(utilDebuff.m_debuffValue);
        stat.m_armor      -= static_cast<int>(armorDebuff.m_debuffValue);
        stat.m_regist     -= static_cast<int>(armorDebuff.m_debuffValue);
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat = CStat(0);
        changeStat.m_strength     = static_cast<int>(attackBuff.m_buffValue);
        changeStat.m_magic        = static_cast<int>(attackBuff.m_buffValue);
        changeStat.m_critical     = static_cast<int>(utilBuff.m_buffValue);
        changeStat.m_speed        = speedBuff.m_buffValue;
        changeStat.m_endure       = static_cast<int>(-utilDebuff.m_debuffValue);
        changeStat.m_armor        = static_cast<int>(-armorDebuff.m_debuffValue);
        changeStat.m_regist       = static_cast<int>(-armorDebuff.m_debuffValue);

        //Skill End Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(attackBuff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }

}