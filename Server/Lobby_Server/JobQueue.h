#pragma once

namespace wod_server{

class IJobQueue
{
public:
    using JobRef = std::shared_ptr<IJob>;

    virtual ~IJobQueue() = default;
    virtual void PushJob(JobRef job) = 0;
    virtual void ProcessJob() = 0;    
};

class JobQueue : public IJobQueue
{
    enum { MAX_JOB_PROCESS = 5 };

public:
    void PushJob(JobRef job) override {
        m_jobQueue.push(job);
    }

    template<typename Func>
    requires std::is_invocable_v<Func&>
    void PushJob(Func&& f) {
        m_jobQueue.push(std::make_shared<Job<Func>>(std::forward<Func>(f)));
    }

    void ProcessJob() override {       
        JobRef job = nullptr;

        int jobcnt = 0;
        while (m_jobQueue.try_pop(job))
        {
            job->Execute();
            if (++jobcnt >= MAX_JOB_PROCESS)
                break;
        }
    }

private:
    concurrency::concurrent_priority_queue<JobRef> m_jobQueue;
};


class PacketJobQueue
{   
public:
    void AddSessionQueue(CClient* object)
    {
        if (object->TryMarkInQueue())
            m_jobQueue.push(object);
    }

    void ProcessJob() {

        while (true)
        {
            CClient* client = nullptr;
            while (m_jobQueue.try_pop(client))
            {
                client->UnmarkInQueue();

                if (client->IsDisconnected())
                {
                    CUserMgr::GetInstance()->ClientReset(client->GetID());
                    continue;
                }

                client->GetJobQueue()->ProcessJob();
            }
        }
    }

private:
    concurrency::concurrent_priority_queue<CClient*> m_jobQueue;
};

}
