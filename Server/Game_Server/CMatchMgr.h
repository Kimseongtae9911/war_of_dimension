#pragma once

namespace wod_server {
	class CMatchMgr : public TSingleton<CMatchMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		CMatch& GetMatch(int _matchnum) { return m_matchs[_matchnum]; }
		const std::array<int, 4>& GetMatchPlayers(int _matchnum) const { return m_matchs[_matchnum].GetClientIds(); }

		void RegisterToMatch(int _match, int _matchID, int _id) { m_matchs[_match].RegisterClient(_matchID, _id); }

	private:
		std::array<CMatch, MAX_MATCH> m_matchs;
	};

}