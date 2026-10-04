#include "pch.h"
#include "OgreRoarHandler.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {

    CSkillHandler* OgreRoarHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreRoarHandler(client);
    }
    void OgreRoarHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreRoar);
        auto attackBuff = skillCsv->buffInfo[EBuffType::AttackIncrease];
        auto slowDebuff = skillCsv->debuffInfo[EDebuffType::Slow];

        CStat clStat = m_client->GetStatus()->GetStat();
        clStat.strength += static_cast<int>(attackBuff.buffValue);
        m_client->GetStatus()->SetStat(clStat);

        CStat changeStat = CStat(0);
        changeStat.strength = static_cast<int>(attackBuff.buffValue);

        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->castingTime), EVENT_TYPE::EV_SKILL_END, -1 });
        CNetworkMgr::GetInstance()->RegisterTimerEvent({ m_client->GetID(), TimeUtil::PassedTimeMSec(attackBuff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < skillCsv->skillRadius) {
                CStat stat = client->GetStatus()->GetStat();
                stat.speed -= slowDebuff.debuffValue;

                CStat changeStat2 = CStat(0);
                changeStat2.speed = -slowDebuff.debuffValue;

                client->GetStatus()->SetStat(stat);
                CNetworkMgr::GetInstance()->RegisterTimerEvent({ client->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat2 });
            }
        }
    }

}