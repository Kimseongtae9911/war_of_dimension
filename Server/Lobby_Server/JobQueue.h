#pragma once
#include <ServerCore/Concurrency.h>
namespace wod_server {
using IJobQueue = wod::core::JobQueue;
class JobQueue : public wod::core::JobQueue {
public:
    JobQueue() : wod::core::JobQueue(wod::core::JobBudget::Five) {}
};
class PacketJobQueue {
public:
    void AddSessionQueue(CClient* _client) {
        std::lock_guard lock(m_mutex);
        if (!m_stopping && _client->TryMarkInQueue()) { m_queue.push(_client); m_ready.notify_one(); }
    }
    void Stop() { std::lock_guard lock(m_mutex); m_stopping = true; m_ready.notify_all(); }
    void ProcessJob() {
        for (;;) {
            CClient* client;
            {
                std::unique_lock lock(m_mutex);
                m_ready.wait(lock, [this] { return m_stopping || !m_queue.empty(); });
                if (m_stopping) return;
                client = m_queue.top(); m_queue.pop();
            }
            if (client->IsDisconnected()) {
                client->GetJobQueue()->Clear(); client->UnmarkInQueue();
                CUserMgr::GetInstance()->ClientReset(client->GetSocketID());
            } else {
                client->GetJobQueue()->ProcessJob(); client->UnmarkInQueue();
                if (client->GetJobQueue()->HasJobs() || client->IsDisconnected()) AddSessionQueue(client);
            }
        }
    }
private:
    std::mutex m_mutex;
    std::condition_variable m_ready;
    std::priority_queue<CClient*> m_queue;
    bool m_stopping = false;
};
}
