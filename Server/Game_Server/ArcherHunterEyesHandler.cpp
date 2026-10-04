#include "pch.h"
#include "ArcherHunterEyesHandler.h"

namespace wod_server {
    ArcherHunterEyesHandler::ArcherHunterEyesHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherHunterEyes;
        SetSkillInfo();
    }

    CSkillHandler* ArcherHunterEyesHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherHunterEyesHandler(client);
    }

    void ArcherHunterEyesHandler::Handle()
    {
        const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);

        m_client->SetUsingSkill(false);
        CStat stat = m_client->GetStatus()->GetStat();
        const auto& buffInfo = skillCsv->buffInfo.find(EBuffType::CriticalIncrease);
        stat.critical += static_cast<int>(buffInfo->second.buffValue);
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat = CStat(0);
        changeStat.critical = static_cast<int>(buffInfo->second.buffValue);

        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
    }

}