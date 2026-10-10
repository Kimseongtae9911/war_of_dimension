#include "pch.h"
#include "FighterPointBloodHandler.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"


namespace wod_server {

    FighterPointBloodHandler::FighterPointBloodHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterPointBlood;
        m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);
        m_castingTime = m_skillCsv->m_buffInfo[EBuffType::UtilIncrease].m_buffDuration;
    }

    CSkillHandler* FighterPointBloodHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterPointBloodHandler(_client);
    }

    void FighterPointBloodHandler::Handle()
    {
        CStat changeStat = CStat(0);
        changeStat.m_speed = m_skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffValue;
        changeStat.m_strength = static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
        changeStat.m_magic = static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
        changeStat.m_critical = static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::UtilIncrease].m_buffValue);

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;

            //My Client
            if (i == m_client->GetMatchId()) {
                CStat stat = m_client->GetStatus()->GetStat();
                stat.m_speed += m_skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffValue;
                stat.m_strength += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
                stat.m_magic += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
                stat.m_critical += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::UtilIncrease].m_buffValue);
                m_client->GetStatus()->SetStat(stat);
                m_client->GetStatus()->m_healthMana.SetCurHp(static_cast<int>(m_client->GetStatus()->m_healthMana.GetCurHp() * (1.0f - m_skillCsv->m_debuffInfo[EDebuffType::MaxHpDecreasePercent].m_debuffValue)));

                for (int id : clientIDs) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_client->GetMatchId(), m_client->GetStatus()->m_healthMana);
                }

                network::GetInstance()->RegisterTimerEvent({ m_client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
                continue;
            }

            //Other Hero Players
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffDistance) {
                //Stat RollBack Event
                CStat stat = client->GetStatus()->GetStat();
                stat.m_speed += m_skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffValue;
                stat.m_strength += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
                stat.m_magic += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
                stat.m_critical += static_cast<int>(m_skillCsv->m_buffInfo[EBuffType::UtilIncrease].m_buffValue);
                client->GetStatus()->SetStat(stat);
                client->GetStatus()->m_healthMana.SetCurHp(static_cast<int>(client->GetStatus()->m_healthMana.GetCurHp() * (1.0f - m_skillCsv->m_debuffInfo[EDebuffType::MaxHpDecreasePercent].m_debuffValue)));

                for (int id : clientIDs) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->m_healthMana);
                }

                network::GetInstance()->RegisterTimerEvent({ client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
            }
        }
    }

}