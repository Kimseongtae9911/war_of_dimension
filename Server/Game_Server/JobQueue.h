#pragma once
#include <ServerCore/Concurrency.h>
namespace wod_server {
using IJobQueue = wod::core::JobQueue;
class JobQueue : public wod::core::JobQueue {
public:
    JobQueue() : wod::core::JobQueue(wod::core::JobBudget::Snapshot) {}
};
}
