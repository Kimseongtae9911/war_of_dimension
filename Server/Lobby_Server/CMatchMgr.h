#pragma once

struct MatchInfo {
    int id;
    unsigned int register_time;

    constexpr bool operator < (const MatchInfo& L) const
    {
        return (register_time > L.register_time);
    }
};

namespace wod_server {

    class CMatchMgr : public TSingleton<CMatchMgr>
    {
    public:
        bool Initialize() override;
        bool Release() override;

        void IncreaseMatchPlayers(char character) { m_players[character]++; }
        void DecreaseMatchPlayers(char character) { m_players[character]--; }

        void RegisterToQue(int id, char character, unsigned int time);

        const bool GetMatch() const;
        const int GetMatchPlayer();
        const int GetMatchBoss();

    private:
        std::array<int, 2> m_players = {};
        std::array<std::priority_queue<MatchInfo>, 2> m_matchque = {};
    };

}
using match = wod_server::CMatchMgr;