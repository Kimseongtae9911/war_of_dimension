#include "pch.h"
#include "CDimensionCrush.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"
#include "CNpc.h"

namespace wod_server {

	CDimensionCrush::CDimensionCrush()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreDimensionCrush);
	}

	CDimensionCrush::~CDimensionCrush()
	{
	}

	bool CDimensionCrush::Update(float _elapsedTime)
	{
		if (!m_active) {
			m_areaLock.lock();
			if (!m_area.empty()) {
				std::unordered_set<int> tempArea = m_area;
				m_areaLock.unlock();
				for (int id : tempArea) {
					if (id >= NPC_ID)
						continue;
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
					CStat stat = client->GetStatus()->GetStat();

					stat.m_speed += m_skillCsv->m_debuffInfo[EDebuffType::Slow].m_debuffValue;
					client->GetStatus()->SetStat(stat);
					client->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;

					for (int playerID : CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum)) {
						if (playerID == -1)
							continue;
						CPacketSender* pkSender = CObjectMgr::GetInstance()->GetClient(playerID)->GetPacketSender();
						pkSender->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::NONE));
						pkSender->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
						pkSender->SendRemoveSkillObjectPacket(m_id, m_type);
					}
				}
				m_areaLock.lock();
				m_area.clear();
				m_areaLock.unlock();
			}
			else {
				m_areaLock.unlock();
			}
			return false;
		}

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);

		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i])
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (DistanceXZ(client->GetPos(), m_pos) < m_skillCsv->m_extraParam1) {
				//Inside Dimension Crush
				m_areaLock.lock();
				if (!m_area.contains(clientIDs[i])) {
					m_areaLock.unlock();
					//Came Into Dimension Crush this frame
					CStat stat = client->GetStatus()->GetStat();
					stat.m_speed -= m_skillCsv->m_debuffInfo[EDebuffType::Slow].m_debuffValue;
					client->GetStatus()->SetStat(stat);
					client->GetStatus()->m_skillBuff = SKILL_BUFF::SILENCE;
					client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);

					for (int j = 0; j < MAX_PLAYER; ++j) {
						if (-1 == clientIDs[j])
							continue;
						CPacketSender* pkSender = CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender();
						pkSender->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::SILENCE));
						pkSender->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
					}

					m_area.insert(clientIDs[i]);

					m_lastDamageTime.insert({ clientIDs[i], TimeUtil::CurTime() });
				}
				else {
					m_areaLock.unlock();
					//Was in Dimension Crush prev frame
					if (TimeUtil::CurTime() - m_lastDamageTime[client->GetID()] > std::chrono::milliseconds(m_skillCsv->m_damageCycleTime)) {
						client->Damage(m_power, m_critical, DAMAGE_TYPE::MAGIC, m_clientID);
						m_lastDamageTime[client->GetID()] = TimeUtil::CurTime();
					}
				}
			}
			else {
				//Outside Dimension Crush
				m_areaLock.lock();
				if (m_area.contains(clientIDs[i])) {
					m_areaLock.unlock();
					//Went out of Dimension Crush this frame
					CStat stat = client->GetStatus()->GetStat();
					stat.m_speed += m_skillCsv->m_debuffInfo[EDebuffType::Slow].m_debuffValue;
					client->GetStatus()->SetStat(stat);
					client->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;
					for (int j = 0; j < MAX_PLAYER; ++j) {
						if (clientIDs[j] == -1)
							continue;
						CPacketSender* pkSender = CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender();
						pkSender->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::NONE));
						pkSender->SendPlayerStatChangePacket(client->GetMatchId(), client->GetStatus()->GetStat());
					}
					m_areaLock.lock();
					m_area.erase(clientIDs[i]);
					m_areaLock.unlock();
					m_lastDamageTime.erase(clientIDs[i]);
				}
				else {
					m_areaLock.unlock();
				}
			}
		}

		for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
			std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(m_matchNum, i);
			if (!npc->m_active)
				continue;
			if (DistanceXZ(npc->GetPos(), m_pos) < m_skillCsv->m_extraParam1) {
				m_areaLock.lock();
				if (m_area.contains(npc->GetID())) {
					m_areaLock.unlock();
					if (TimeUtil::CurTime() - m_lastDamageTime[npc->GetID()] > std::chrono::milliseconds(m_skillCsv->m_damageCycleTime)) {
						npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC, false);
						m_lastDamageTime[npc->GetID()] = TimeUtil::CurTime();
					}
				}
				else {
					m_area.insert(npc->GetID());
					m_areaLock.unlock();
					m_lastDamageTime.insert({ npc->GetID(), TimeUtil::CurTime() });
					npc->Damaged(m_clientID, m_power, DAMAGE_TYPE::MAGIC, false);
				}
			}
			else {
				m_areaLock.lock();
				if (m_area.contains(npc->GetID())) {
					m_area.erase(npc->GetID());
					m_areaLock.unlock();
					m_lastDamageTime.erase(npc->GetID());
				}
				else
					m_areaLock.unlock();
			}
		}

		return true;
	}

}