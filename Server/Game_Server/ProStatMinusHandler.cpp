#include "pch.h"
#include "ProStatMinusHandler.h"
#include "CNetworkMgr.h"
#include "CObjectMgr.h"
#include "CMatchMgr.h"

namespace wod_server {

    CSkillHandler* ProStatMinusHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProStatMinusHandler(client);
    }

    void ProStatMinusHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPlusStats);
        auto slowDebuff = skillCsv->debuffInfo[EDebuffType::Slow];
        auto statDebuff = skillCsv->debuffInfo[EDebuffType::StatDecrease];

        CStat changeStat(0);
        changeStat.armor =      static_cast<int>(-statDebuff.debuffValue);
        changeStat.critical =   static_cast<int>(-statDebuff.debuffValue);
        changeStat.endure =     static_cast<int>(-statDebuff.debuffValue);
        changeStat.magic =      static_cast<int>(-statDebuff.debuffValue);
        changeStat.regist =     static_cast<int>(-statDebuff.debuffValue);
        changeStat.strength =   static_cast<int>(-statDebuff.debuffValue);
        changeStat.speed = -slowDebuff.debuffValue;

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;

            std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);

            if (DistanceXZ(hero->GetPos(), m_client->GetPos()) < skillCsv->skillRadius) {
                CStat stat = hero->GetStatus()->GetStat();
                stat.armor -= static_cast<int>(statDebuff.debuffValue);
                stat.critical -= static_cast<int>(statDebuff.debuffValue);
                stat.endure -= static_cast<int>(statDebuff.debuffValue);
                stat.magic -= static_cast<int>(statDebuff.debuffValue);
                stat.regist -= static_cast<int>(statDebuff.debuffValue);
                stat.strength -= static_cast<int>(statDebuff.debuffValue);
                stat.speed -= slowDebuff.debuffValue;
                hero->GetStatus()->SetStat(stat);
                network::GetInstance()->RegisterTimerEvent({ hero->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
            }
        }       
    }

}