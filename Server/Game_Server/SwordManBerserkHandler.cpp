#include "pch.h"
#include "SwordManBerserkHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    CSkillHandler* SwordManBerserkHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManBerserkHandler(client);
    }

    void SwordManBerserkHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManBerserk);
        auto attackBuff = skillCsv->buffInfo[EBuffType::AttackIncrease];
        auto utilBuff = skillCsv->buffInfo[EBuffType::UtilIncrease];
        auto speedBuff = skillCsv->buffInfo[EBuffType::SpeedIncrease];

        auto armorDebuff = skillCsv->debuffInfo[EDebuffType::ArmorDecrease];
        auto utilDebuff = skillCsv->debuffInfo[EDebuffType::UtilDecrease];

        CStat stat = m_client->GetStatus()->GetStat();
        stat.strength   += static_cast<int>(attackBuff.buffValue);
        stat.magic      += static_cast<int>(attackBuff.buffValue);
        stat.critical   += static_cast<int>(utilBuff.buffValue);
        stat.speed      += speedBuff.buffValue;
        stat.endure     -= static_cast<int>(utilDebuff.debuffValue);
        stat.armor      -= static_cast<int>(armorDebuff.debuffValue);
        stat.regist     -= static_cast<int>(armorDebuff.debuffValue);
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat = CStat(0);
        changeStat.strength     = static_cast<int>(attackBuff.buffValue);
        changeStat.magic        = static_cast<int>(attackBuff.buffValue);
        changeStat.critical     = static_cast<int>(utilBuff.buffValue);
        changeStat.speed        = speedBuff.buffValue;
        changeStat.endure       = static_cast<int>(-utilDebuff.debuffValue);
        changeStat.armor        = static_cast<int>(-armorDebuff.debuffValue);
        changeStat.regist       = static_cast<int>(-armorDebuff.debuffValue);

        //Skill End Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(attackBuff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
    }

}