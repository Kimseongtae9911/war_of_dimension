#include "pch.h"
#include "SwordManWarCryHandler.h"

namespace wod_server {
    CSkillHandler* SwordManWarCryHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManWarCryHandler(_client);
    }

    void SwordManWarCryHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManWarCry);
        auto buff = skillCsv->m_buffInfo[EBuffType::SpeedIncrease];

        CStat changeStat = CStat(0);
        changeStat.m_speed = buff.m_buffValue;

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;
            if (i == m_client->GetMatchId()) {
                CStat stat = m_client->GetStatus()->GetStat();
                stat.m_speed += buff.m_buffValue;
                m_client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(buff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
                continue;
            }
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < buff.m_buffDistance) {
                //Stat RollBack Event
                CStat stat = client->GetStatus()->GetStat();
                stat.m_speed += buff.m_buffValue;
                client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ client->GetID(), TimeUtil::PassedTimeMSec(buff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
            }
        }
    }

}