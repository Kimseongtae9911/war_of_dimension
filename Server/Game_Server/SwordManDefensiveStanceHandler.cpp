#include "pch.h"
#include "SwordManDefensiveStanceHandler.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {

    CSkillHandler* SwordManDefensiveStanceHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManDefensiveStanceHandler(_client);
    }

    void SwordManDefensiveStanceHandler::Handle()
    {
        auto slowDebuff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManDefensiveStance)->m_debuffInfo[EDebuffType::Slow];
        if (m_client->GetStatus()->m_defensiveBuff == DEFENSIVE_BUFF::DEFENSIVE_STANCE) {
            m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;
            CStat stat = m_client->GetStatus()->GetStat();
            stat.m_speed -= slowDebuff.m_debuffValue;
            m_client->GetStatus()->SetStat(stat);

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
                if (-1 == id)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
            }
        }
        else {
            m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::DEFENSIVE_STANCE;
            CStat stat = m_client->GetStatus()->GetStat();
            stat.m_speed += slowDebuff.m_debuffValue;
            m_client->GetStatus()->SetStat(stat);

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
                if (-1 == id)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
            }
        }
    }

}