#include "pch.h"
#include "SwordManDefensiveStanceHandler.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {

    CSkillHandler* SwordManDefensiveStanceHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManDefensiveStanceHandler(client);
    }

    void SwordManDefensiveStanceHandler::Handle()
    {     
        auto slowDebuff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManDefensiveStance)->debuffInfo[EDebuffType::Slow];
        if (m_client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::DEFENSIVE_STANCE) {
            m_client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;
            CStat stat = m_client->GetStatus()->GetStat();
            stat.speed -= slowDebuff.debuffValue;
            m_client->GetStatus()->SetStat(stat);

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
                if (-1 == id)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
            }
        }
        else {
            m_client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::DEFENSIVE_STANCE;
            CStat stat = m_client->GetStatus()->GetStat();
            stat.speed += slowDebuff.debuffValue;
            m_client->GetStatus()->SetStat(stat);

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
                if (-1 == id)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
            }
        }
    }

}