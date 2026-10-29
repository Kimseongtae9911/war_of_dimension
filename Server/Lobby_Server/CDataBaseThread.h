#pragma once
#include "CDataBase.h"
#include "RewardJournal.h"
#include <ServerCore/OrderedWorkQueue.h>
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

    bool Submit(std::function<void(CDataBase&)> _work);
    void SubmitReward(const std::string& _account, int _delta);
    void Stop();
    std::string SnapshotJson() const;

  private:
    void ScheduleReward(RewardJournal::Record _record);
    std::unique_ptr<RewardJournal> m_rewards;
    wod::core::OrderedWorkQueue m_rewardIngress;
    std::thread m_rewardWriter;
    wod::core::OrderedWorkQueue m_queue;
    std::thread m_worker;
    CDataBase* m_database = nullptr; // worker만 접근, join 후 소멸
    bool m_enabled = false;
};
}
