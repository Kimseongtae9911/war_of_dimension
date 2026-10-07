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
    void AddSessionQueue(CClient* client) {
        std::lock_guard lock(mutex_);
        if (!stopping_ && client->TryMarkInQueue()) { queue_.push(client); ready_.notify_one(); }
    }
    void Stop() { std::lock_guard lock(mutex_); stopping_ = true; ready_.notify_all(); }
    void ProcessJob() {
        for (;;) {
            CClient* client;
            {
                std::unique_lock lock(mutex_);
                ready_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
                if (stopping_) return;
                client = queue_.top(); queue_.pop();
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
    std::mutex mutex_;
    std::condition_variable ready_;
    std::priority_queue<CClient*> queue_;
    bool stopping_ = false;
};
}
