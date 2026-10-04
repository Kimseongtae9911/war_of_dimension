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
public:    
    void PushJob(JobRef job) override {
        m_jobQueue.push(job);
    }

    template<typename Func>
    void PushJob(Func&& f) {
        m_jobQueue.push(std::make_shared<Job<Func>>(std::forward<Func>(f)));
    }

    void ProcessJob() override {       
        JobRef job = nullptr;

        int jobSize = m_jobQueue.size();
        for (int i = 0; i < jobSize; ++i)
        {
            if (!m_jobQueue.try_pop(job))
                return;
            job->Execute();
        }            
    }

private:
    concurrency::concurrent_priority_queue<JobRef> m_jobQueue;
};

}