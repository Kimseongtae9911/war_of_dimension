#include "pch.h"
#include "CMatchMgr.h"

namespace wod_server {
	std::unique_ptr<CMatchMgr> CMatchMgr::m_instance;

	bool CMatchMgr::Initialize()
	{
		m_players = { 0, 0 };
		return true;
	}

	bool CMatchMgr::Release()
	{
		return true;
	}

	void CMatchMgr::RegisterToQue(int id, char character, unsigned int time)
	{
		//character 0=normal, 1=boss
		m_matchque[static_cast<int>(character)].push({ id, time });
		m_players[static_cast<int>(character)]++;
	}

	const bool CMatchMgr::GetMatch() const
	{
		if (m_players[0] >= 3 && m_players[1] >= 1)
			return true;		
		return false;
	}

	const int CMatchMgr::GetMatchPlayer()
	{
		int id = m_matchque[0].top().id;
		m_matchque[0].pop();
		m_players[0]--;

		return id;
	}

	const int CMatchMgr::GetMatchBoss()
	{
		int id = m_matchque[1].top().id;
		m_matchque[1].pop();
		m_players[1]--;

		return id;
	}

}