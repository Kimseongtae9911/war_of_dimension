#include "pch.h"
#include "ProStatMinusHandler.h"
#include "CNetworkMgr.h"
#include "CObjectMgr.h"
#include "CMatchMgr.h"

namespace wod_server {

    CSkillHandler* ProStatMinusHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProStatMinusHandler(_client);
    }

    void ProStatMinusHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPlusStats);
        auto slowDebuff = skillCsv->m_debuffInfo[EDebuffType::Slow];
        auto statDebuff = skillCsv->m_debuffInfo[EDebuffType::StatDecrease];

        CStat changeStat(0);
        changeStat.m_armor =      static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_critical =   static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_endure =     static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_magic =      static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_regist =     static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_strength =   static_cast<int>(-statDebuff.m_debuffValue);
        changeStat.m_speed = -slowDebuff.m_debuffValue;

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;

            std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);

            if (DistanceXZ(hero->GetPos(), m_client->GetPos()) < skillCsv->m_skillRadius) {
                CStat stat = hero->GetStatus()->GetStat();
                stat.m_armor -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_critical -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_endure -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_magic -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_regist -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_strength -= static_cast<int>(statDebuff.m_debuffValue);
                stat.m_speed -= slowDebuff.m_debuffValue;
                hero->GetStatus()->SetStat(stat);
                network::GetInstance()->RegisterTimerEvent({ hero->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
            }
        }
    }

}