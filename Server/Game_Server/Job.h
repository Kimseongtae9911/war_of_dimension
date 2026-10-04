#pragma once

class IJob {
public:
    virtual ~IJob() = default;
    virtual void Execute() = 0;
};


template<typename Func>
class Job : public IJob {
public:
    Job(Func&& f) : func(std::forward<Func>(f)), timestamp(std::chrono::high_resolution_clock::now()) {}
    void Execute() override {
        func();
    }

    Job(Func&& f, std::chrono::high_resolution_clock::time_point time) : func(std::forward<Func>(f)), timestamp(time) {}

    bool operator<(const Job& other) const {
        return timestamp > other.timestamp;
    }

private:
    Func func;
    std::chrono::high_resolution_clock::time_point timestamp;
};

