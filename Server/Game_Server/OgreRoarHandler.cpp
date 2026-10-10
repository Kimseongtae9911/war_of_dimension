#include "pch.h"
#include "OgreRoarHandler.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {

    CSkillHandler* OgreRoarHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreRoarHandler(_client);
    }
    void OgreRoarHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreRoar);
        auto attackBuff = skillCsv->m_buffInfo[EBuffType::AttackIncrease];
        auto slowDebuff = skillCsv->m_debuffInfo[EDebuffType::Slow];

        CStat clStat = m_client->GetStatus()->GetStat();
        clStat.m_strength += static_cast<int>(attackBuff.m_buffValue);
        m_client->GetStatus()->SetStat(clStat);

        CStat changeStat = CStat(0);
        changeStat.m_strength = static_cast<int>(attackBuff.m_buffValue);

        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(attackBuff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < skillCsv->m_skillRadius) {
                CStat stat = client->GetStatus()->GetStat();
                stat.m_speed -= slowDebuff.m_debuffValue;

                CStat changeStat2 = CStat(0);
                changeStat2.m_speed = -slowDebuff.m_debuffValue;

                client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ client->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat2 });
            }
        }
    }

}