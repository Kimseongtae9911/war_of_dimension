#include "pch.h"
#include "ArcherHunterEyesHandler.h"

namespace wod_server {
    ArcherHunterEyesHandler::ArcherHunterEyesHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherHunterEyes;
        SetSkillInfo();
    }

    CSkillHandler* ArcherHunterEyesHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherHunterEyesHandler(_client);
    }

    void ArcherHunterEyesHandler::Handle()
    {
        const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);

        m_client->SetUsingSkill(false);
        CStat stat = m_client->GetStatus()->GetStat();
        const auto& buffInfo = skillCsv->m_buffInfo.find(EBuffType::CriticalIncrease);
        stat.m_critical += static_cast<int>(buffInfo->second.m_buffValue);
        m_client->GetStatus()->SetStat(stat);

        CStat changeStat = CStat(0);
        changeStat.m_critical = static_cast<int>(buffInfo->second.m_buffValue);

        //Stat RollBack Event
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
    }

}