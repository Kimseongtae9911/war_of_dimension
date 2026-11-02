#pragma once
#include "CDataBase.h"
#include "RewardJournal.h"
#include <ServerCore/OrderedWorkQueue.h>
#include <ServerCore/RecoveringWorkQueue.h>
#include <thread>

namespace wod_server
{
class CDataBaseThread
{
  public:
    CDataBaseThread();
    ~CDataBaseThread();

    bool IsEnabled() const
    {
        return m_enabled;
    }

    using Failure = wod::core::RecoveringWorkQueue::Failure;
    bool Submit(std::function<void(CDataBase&)> _work, std::function<void(Failure)> _failure = {}, bool _replaySafe = false, std::function<bool()> _valid = {});
    void SubmitReward(const std::string& _account, int _delta);
    void Stop();
    std::string SnapshotJson() const;

  private:
    void ScheduleReward(RewardJournal::Record _record);
    std::unique_ptr<RewardJournal> m_rewards;
    wod::core::OrderedWorkQueue m_rewardIngress;
    std::thread m_rewardWriter;
    wod::core::RecoveringWorkQueue m_queue;
    std::thread m_worker;
    CDataBase* m_database = nullptr; // worker만 접근, join 후 소멸
    bool m_enabled = false;
};
}
