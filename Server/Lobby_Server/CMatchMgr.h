#pragma once

struct MatchInfo {
    int m_id;
    unsigned int m_register_time;

    constexpr bool operator < (const MatchInfo& _l) const
    {
        return (m_register_time > _l.m_register_time);
    }
};

namespace wod_server {

    class CMatchMgr : public TSingleton<CMatchMgr>
    {
    public:
        bool Initialize() override;
        bool Release() override;

        void IncreaseMatchPlayers(char _character) { m_players[_character]++; }
        void DecreaseMatchPlayers(char _character) { m_players[_character]--; }

        void RegisterToQue(int _id, char _character, unsigned int _time);

        const bool GetMatch() const;
        const int GetMatchPlayer();
        const int GetMatchBoss();

    private:
        std::array<int, 2> m_players = {};
        std::array<std::priority_queue<MatchInfo>, 2> m_matchque = {};
    };

}
using match = wod_server::CMatchMgr;