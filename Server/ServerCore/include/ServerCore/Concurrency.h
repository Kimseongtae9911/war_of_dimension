#pragma once
#include <chrono>
#include <atomic>
#include <thread>
#include <vector>
#include <concurrent_priority_queue.h>
#include <condition_variable>
#include <functional>
#include <exception>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <type_traits>
#include <unordered_map>

namespace wod::core {
// producer 그룹을 먼저 join한 뒤 IOCP 그룹을 정리한다. 부분 생성 실패도 같은 경로다.
class ThreadGroup {
public:
    using Failure = std::function<void(std::exception_ptr)>;
    ThreadGroup(std::function<void()> stop, Failure failure) : stop_(std::move(stop)), failure_(std::move(failure)) {}
    ~ThreadGroup() { StopAndJoin(); }
    template<class Func> void Launch(Func&& func) {
        threads_.emplace_back([this, work=std::forward<Func>(func)]() mutable {
            try { std::invoke(work); }
            catch (...) { failed_=true; failure_(std::current_exception()); }
        });
    }
    void StopAndJoin() {
        if (joined_) return;
        stop_();
        for (auto& thread : threads_) if (thread.joinable()) thread.join();
        joined_=true;
    }
    bool Failed() const { return failed_.load(); }
private:
    std::function<void()> stop_;
    Failure failure_;
    std::vector<std::thread> threads_;
    std::atomic_bool failed_=false;
    bool joined_=false;
};
class IJob {
public:
    virtual ~IJob() = default;
    virtual void Execute() = 0;
};
template<class Func> class Job final : public IJob {
public:
    explicit Job(Func f) : func_(std::move(f)) {}
    Job(Func f, std::chrono::high_resolution_clock::time_point time) : func_(std::move(f)), time_(time) {}
    void Execute() override { std::invoke(func_); }
    bool operator<(const Job& other) const { return time_ > other.time_; }
private:
    Func func_;
    std::chrono::high_resolution_clock::time_point time_ = std::chrono::high_resolution_clock::now();
};
enum class JobBudget { Snapshot, Five };
class JobQueue {
public:
    using JobRef = std::shared_ptr<IJob>;
    explicit JobQueue(JobBudget budget = JobBudget::Snapshot) : budget_(budget) {}
    virtual ~JobQueue() = default;
    void PushJob(JobRef job) { if (job) jobs_.push(std::move(job)); }
    template<class Func> requires std::is_invocable_v<std::decay_t<Func>&>
    void PushJob(Func&& f) { PushJob(std::make_shared<Job<std::decay_t<Func>>>(std::forward<Func>(f))); }
    virtual void ProcessJob() {
        std::lock_guard execute(execute_);
        const auto count = budget_ == JobBudget::Five ? size_t{5} : jobs_.size();
        for (size_t i = 0; i < count; ++i) {
            JobRef job;
            if (!jobs_.try_pop(job)) break;
            job->Execute();
        }
    }
    bool HasJobs() const { return !jobs_.empty(); }
    void Clear() { std::lock_guard execute(execute_); JobRef job; while (jobs_.try_pop(job)) {} }
private:
    // 기존 shared_ptr 우선순위 비교를 유지한다. FIFO/시간순 계약이 아니다.
    concurrency::concurrent_priority_queue<JobRef> jobs_;
    JobBudget budget_;
    std::mutex execute_;
};

template<class T> class ObjectPool {
public:
    T* Acquire() {
        std::lock_guard lock(mutex_);
        if (available_.empty()) {
            auto value = std::make_unique<T>();
            auto* ptr = value.get();
            owned_.emplace(ptr, std::move(value));
            ++leased_;
            return ptr;
        }
        T* ptr = available_.top(); available_.pop(); free_.erase(ptr); ++leased_; return ptr;
    }
    bool try_pop(T*& ptr) {
        std::lock_guard lock(mutex_);
        if (available_.empty()) return false;
        ptr = available_.top(); available_.pop(); free_.erase(ptr); ++leased_; return true;
    }
    void push(T* ptr) {
        if (!ptr) return;
        std::lock_guard lock(mutex_);
        if (free_.contains(ptr)) throw std::logic_error("I/O pool double return");
        ptr->Reset();
        if (!owned_.contains(ptr)) owned_.emplace(ptr, std::unique_ptr<T>(ptr));
        else { if (leased_ == 0) throw std::logic_error("I/O pool lease underflow"); --leased_; }
        free_.emplace(ptr, true); available_.push(ptr);
    }
    size_t Leased() const { std::lock_guard lock(mutex_); return leased_; }
    size_t Size() const { std::lock_guard lock(mutex_); return owned_.size(); }
    void Clear() {
        std::lock_guard lock(mutex_);
        if (leased_) throw std::logic_error("I/O pool still leased");
        available_ = {}; free_.clear(); owned_.clear();
    }
private:
    mutable std::mutex mutex_;
    std::priority_queue<T*> available_;
    std::unordered_map<T*, std::unique_ptr<T>> owned_;
    std::unordered_map<T*, bool> free_;
    size_t leased_ = 0;
};
}
