#pragma once

namespace wod_server
{
// 전역 조정 큐의 배타 구간에서만 접근한다.
class CMatchMgr : public TSingleton<CMatchMgr>
{
  public:
    bool Initialize() override;
    bool Release() override;
    void RegisterToQue(int _id, char _character, unsigned int _time);
    void Cancel(int _id);
    bool TryMakeMatch(std::array<int, 4>& _players);

  private:
    struct Entry
    {
        int m_id;
        unsigned int m_time;
        uint64_t m_generation;
    };

    std::array<std::vector<Entry>, 2> m_waiting;
};
}

using match = wod_server::CMatchMgr;
