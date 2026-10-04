#pragma once

namespace wod_server {	
	class CMatchMgr : public TSingleton<CMatchMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		CMatch& GetMatch(int matchnum) { return m_matchs[matchnum]; }
		const std::array<int, 4>& GetMatchPlayers(int matchnum) const { return m_matchs[matchnum].GetClientIds(); }

		void RegisterToMatch(int match, int matchID, int id) { m_matchs[match].RegisterClient(matchID, id); }

	private:
		std::array<CMatch, MAX_MATCH> m_matchs;
	};

}