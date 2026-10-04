#include "pch.h"
#include "CProtectedArea.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"

namespace wod_server {

	CProtectedArea::CProtectedArea()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManProtectedArea);
	}

	CProtectedArea::~CProtectedArea()
	{
	}

	bool CProtectedArea::Update(float elapsedTime)
	{		
		if (!active) {
			m_areaLock.lock();
			if (!m_area.empty()) {
				std::unordered_set<int> tempArea = m_area;
				m_areaLock.unlock();
				for (int id : tempArea) {
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
					CStat stat = client->GetStatus()->GetStat();
					stat.armor -= m_defensePower;
					client->GetStatus()->SetStat(stat);
				}
				m_areaLock.lock();
				m_area.clear();
				m_areaLock.unlock();
			}
			else
				m_areaLock.unlock();
			return false;
		}

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);

		for (int i = 0; i < 3; ++i) {
			if (clientIDs[i] == -1)
				continue;
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
			if (DistanceXZ(client->GetPos(), m_pos) < m_skillCsv->buffInfo[EBuffType::DefenseIncrease].buffDistance) {
				//Inside Protected Area
				m_areaLock.lock();
				if (!m_area.contains(clientIDs[i])) {
					//Came Into Protected Area this frame
					m_area.insert(clientIDs[i]);
					m_areaLock.unlock();

					CStat stat = client->GetStatus()->GetStat();
					stat.armor += m_defensePower;
					client->GetStatus()->SetStat(stat);
				}
				else
					m_areaLock.unlock();
			}
			else {
				//Outside Protected Area
				m_areaLock.lock();
				if (m_area.contains(clientIDs[i])) {
					//Went out of Protected Area this frame
					m_area.erase(clientIDs[i]);
					m_areaLock.unlock();
					CStat stat = client->GetStatus()->GetStat();
					stat.armor -= m_defensePower;
					client->GetStatus()->SetStat(stat);
				}
				else
					m_areaLock.unlock();
			}
		}

		return true;
	}
}