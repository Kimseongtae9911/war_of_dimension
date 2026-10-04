#include "pch.h"
#include "FighterPointBloodHandler.h"
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"


namespace wod_server {

    FighterPointBloodHandler::FighterPointBloodHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::FighterPointBlood;
        m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);
        m_castingTime = m_skillCsv->buffInfo[EBuffType::UtilIncrease].buffDuration;
    }

    CSkillHandler* FighterPointBloodHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterPointBloodHandler(client);
    }

    void FighterPointBloodHandler::Handle()
    {
        CStat changeStat = CStat(0);
        changeStat.speed = m_skillCsv->buffInfo[EBuffType::SpeedIncrease].buffValue;
        changeStat.strength = static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
        changeStat.magic = static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
        changeStat.critical = static_cast<int>(m_skillCsv->buffInfo[EBuffType::UtilIncrease].buffValue);

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;
            
            //My Client
            if (i == m_client->GetMatchId()) {
                CStat stat = m_client->GetStatus()->GetStat();
                stat.speed += m_skillCsv->buffInfo[EBuffType::SpeedIncrease].buffValue;
                stat.strength += static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
                stat.magic += static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
                stat.critical += static_cast<int>(m_skillCsv->buffInfo[EBuffType::UtilIncrease].buffValue);
                m_client->GetStatus()->SetStat(stat);
                m_client->GetStatus()->healthMana.SetCurHp(static_cast<int>(m_client->GetStatus()->healthMana.GetCurHp() * (1.0f - m_skillCsv->debuffInfo[EDebuffType::MaxHpDecreasePercent].debuffValue)));

                for (int id : clientIDs) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_client->GetMatchId(), m_client->GetStatus()->GetStat());
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_client->GetMatchId(), m_client->GetStatus()->healthMana);
                }

                network::GetInstance()->RegisterTimerEvent({ m_client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
                continue;
            }

            //Other Hero Players
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
            if (DistanceXZ(m_client->GetPos(), client->GetPos()) < m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffDistance) {
                //Stat RollBack Event
                CStat stat = client->GetStatus()->GetStat();
                stat.speed += m_skillCsv->buffInfo[EBuffType::SpeedIncrease].buffValue;
                stat.strength += static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
                stat.magic += static_cast<int>(m_skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
                stat.critical += static_cast<int>(m_skillCsv->buffInfo[EBuffType::UtilIncrease].buffValue);
                client->GetStatus()->SetStat(stat);
                client->GetStatus()->healthMana.SetCurHp(static_cast<int>(client->GetStatus()->healthMana.GetCurHp() * (1.0f - m_skillCsv->debuffInfo[EDebuffType::MaxHpDecreasePercent].debuffValue)));

                for (int id : clientIDs) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
                }

                network::GetInstance()->RegisterTimerEvent({ client->GetID(), SkillUseTime(), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
            }
        }
    }

}