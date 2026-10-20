#include "pch.h"
#include "CMatchMgr.h"
#include "CUserMgr.h"

namespace wod_server
{
std::unique_ptr<CMatchMgr> CMatchMgr::m_instance;

bool CMatchMgr::Initialize()
{
    m_waiting = {};

    return true;
}

bool CMatchMgr::Release()
{
    m_waiting = {};

    return true;
}

void CMatchMgr::Cancel(int _id)
{
    for (auto& queue : m_waiting)
        std::erase_if(queue, [_id](const Entry& _entry) {
            return _entry.m_id == _id;
        });
}

void CMatchMgr::RegisterToQue(int _id, char _character, unsigned int _time)
{
    if (_character < 0 || _character > 1)
        return;

    Cancel(_id);
    auto* client = CUserMgr::GetInstance()->GetClient(_id);
    m_waiting[_character].push_back({_id, _time, client->GetPacketSender()->GetSession()->Generation()});
}

bool CMatchMgr::TryMakeMatch(std::array<int, 4>& _players)
{
    for (auto& queue : m_waiting)
    {
        std::erase_if(queue, [](const Entry& _entry) {
            auto* client = CUserMgr::GetInstance()->GetClient(_entry.m_id);

            return client->IsDisconnected() || client->GetState() != CL_STATE::ST_MATCHING || client->GetPacketSender()->GetSession()->Generation() != _entry.m_generation;
        });
        std::stable_sort(queue.begin(), queue.end(), [](const Entry& _left, const Entry& _right) {
            return _left.m_time < _right.m_time;
        });
    }

    if (m_waiting[0].size() < 3 || m_waiting[1].empty())
        return false;

    for (int i = 0; i < 3; ++i)
        _players[i] = m_waiting[0][i].m_id;

    _players[3] = m_waiting[1].front().m_id;
    m_waiting[0].erase(m_waiting[0].begin(), m_waiting[0].begin() + 3);
    m_waiting[1].erase(m_waiting[1].begin());

    return true;
}
}
