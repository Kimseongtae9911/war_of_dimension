#include "pch.h"
#include "CHelloWorld.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "CClient.h"

namespace wod_server {

	CHelloWorld::CHelloWorld()
	{
		m_skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerHelloWorld);
	}

	CHelloWorld::~CHelloWorld()
	{
	}

	bool CHelloWorld::Update(float _elapsedTime)
	{
		if (!m_active) {
			m_area.clear();
			return false;
		}

		std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_matchNum);
		for (int id : m_area) {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(id);
			if (DistanceXZ(client->GetPos(), m_pos) > m_skillCsv->m_skillRadius) {
				client->SetPos(m_pos);
				for (int clid : clientIDs) {
					if (-1 == clid)
						continue;
					CObjectMgr::GetInstance()->GetClient(clid)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), 0);
				}
			}
		}

		for (int i = 0; i < MAX_PLAYER - 1; ++i) {
			if (-1 == clientIDs[i] || m_area.contains(clientIDs[i]))
				continue;
			if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos(), m_pos) < m_skillCsv->m_skillRadius) {
				m_area.insert(clientIDs[i]);
			}
		}

		return true;
	}

	void CHelloWorld::SetArea(const std::vector<int>& _ids)
	{
		for (int id : _ids) {
			m_area.insert(id);
		}
	}
}