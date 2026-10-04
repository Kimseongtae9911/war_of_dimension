#include "pch.h"
#include "ArcherWindStepHandler.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {
    ArcherWindStepHandler::ArcherWindStepHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherWindStep;
        SetSkillInfo();
    }

    CSkillHandler* ArcherWindStepHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherWindStepHandler(client);
    }

    void ArcherWindStepHandler::Handle()
    {
        const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);
        const auto& speedBuff = skillCsv->buffInfo.find(EBuffType::SpeedIncrease)->second;
        m_client->SetUsingSkill(false);
        CStat changeStat = CStat(0);
        changeStat.speed = speedBuff.buffValue;

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (-1 == clientIDs[i])
                continue;

            if (i == m_client->GetMatchId()) {
                CStat stat = m_client->GetStatus()->GetStat();
                stat.speed += speedBuff.buffValue;
 
                m_client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
                continue;
            }
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < speedBuff.buffDistance) {
                //Stat RollBack Event
                CStat stat = client->GetStatus()->GetStat();
                stat.speed += speedBuff.buffValue;

                client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
            }
        }
    }

}