#pragma once
#include "CDataBase.h"
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
    void Stop();
    std::string SnapshotJson() const;

  private:
    wod::core::OrderedWorkQueue m_queue;
    std::thread m_worker;
    CDataBase* m_database = nullptr; // worker만 접근, join 후 소멸
    bool m_enabled = false;
};
}
