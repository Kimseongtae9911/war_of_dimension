#include "../../Lobby_Server/LobbyDispatcher.h"
#include <future>
#include <array>

namespace
{
using namespace std::chrono_literals;
using wod_server::LobbyDispatcher;

void Check(bool _condition, const char* _message)
{
    if (!_condition)
        throw std::runtime_error(_message);
}

void ParallelAndSerial()
{
    LobbyDispatcher dispatcher(2);
    LobbyDispatcher::SessionWorkQueue a, same, b;
    a.SetChannel(0);
    same.SetChannel(0);
    b.SetChannel(1);
    std::promise<void> entered, other, serial, release;
    auto block = release.get_future().share();
    auto first = entered.get_future();
    auto second = other.get_future();
    auto third = serial.get_future();
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    workers.Launch([&] {
        dispatcher.ProcessJob();
    });
    workers.Launch([&] {
        dispatcher.ProcessJob();
    });
    dispatcher.Post(a, false, [&] {
        entered.set_value();
        block.wait();
    });
    const bool started = first.wait_for(2s) == std::future_status::ready;
    dispatcher.Post(same, false, [&] {
        serial.set_value();
    });
    dispatcher.Post(b, false, [&] {
        other.set_value();
    });
    const bool parallel = second.wait_for(2s) == std::future_status::ready;
    const bool held = third.wait_for(20ms) == std::future_status::timeout;
    release.set_value();
    const bool finished = third.wait_for(2s) == std::future_status::ready;
    workers.StopAndJoin();
    dispatcher.Clear();
    Check(started && parallel && held && finished, "channel parallelism and same-channel exclusion");
    const auto stats = dispatcher.SnapshotJson();
    Check(stats.find("\"max_parallel\":2") != std::string::npos && stats.find("\"owner_violations\":0") != std::string::npos, "ownership telemetry");
}

void MigrationOrder()
{
    LobbyDispatcher dispatcher(2);
    LobbyDispatcher::SessionWorkQueue workQueue;
    workQueue.SetChannel(0);
    std::vector<int> received;
    std::promise<void> done;
    auto completion = done.get_future();
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    for (int i = 0; i < 4; ++i)
        workers.Launch([&] {
            dispatcher.ProcessJob();
        });

    for (int i = 0; i < 200; ++i)
    {
        dispatcher.Post(workQueue, i % 5 == 0, [&, i] {
            if (i % 5 == 0)
                workQueue.SetChannel((i / 5) % 2);

            received.push_back(i);
            if (i == 199)
                done.set_value();
        });
    }

    const bool finished = completion.wait_for(3s) == std::future_status::ready;
    workers.StopAndJoin();
    dispatcher.Clear();
    Check(finished && received.size() == 200, "migration completion");
    for (int i = 0; i < 200; ++i)
        Check(received[i] == i, "migration preserves admission FIFO across channels/global queue");

    Check(!dispatcher.Post(workQueue, false,
                           [] {
                           }) &&
              !dispatcher.PostGlobal([] {
              }),
          "stopped dispatcher rejects producers");
}

void GlobalBarrier()
{
    LobbyDispatcher dispatcher(2);
    LobbyDispatcher::SessionWorkQueue a, b;
    a.SetChannel(0);
    b.SetChannel(1);
    std::promise<void> channelEntered, globalEntered, channelDone, releaseChannel, releaseGlobal;
    auto channelReady = channelEntered.get_future();
    auto globalReady = globalEntered.get_future();
    auto done = channelDone.get_future();
    auto firstBlock = releaseChannel.get_future().share();
    auto secondBlock = releaseGlobal.get_future().share();
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    for (int i = 0; i < 3; ++i)
        workers.Launch([&] {
            dispatcher.ProcessJob();
        });

    dispatcher.Post(a, false, [&] {
        channelEntered.set_value();
        firstBlock.wait();
    });
    const bool started = channelReady.wait_for(2s) == std::future_status::ready;
    dispatcher.PostGlobal([&] {
        globalEntered.set_value();
        secondBlock.wait();
    });
    const bool globalWaited = globalReady.wait_for(20ms) == std::future_status::timeout;
    releaseChannel.set_value();
    const bool globalStarted = globalReady.wait_for(2s) == std::future_status::ready;
    dispatcher.Post(b, false, [&] {
        channelDone.set_value();
    });
    const bool channelWaited = done.wait_for(20ms) == std::future_status::timeout;
    releaseGlobal.set_value();
    const bool finished = done.wait_for(2s) == std::future_status::ready;
    workers.StopAndJoin();
    dispatcher.Clear();
    Check(started && globalWaited && globalStarted && channelWaited && finished, "global and channel work exclude each other");
}

void StopWithPending()
{
    LobbyDispatcher dispatcher(1);
    LobbyDispatcher::SessionWorkQueue workQueue;
    workQueue.SetChannel(0);
    std::promise<void> entered, release;
    auto ready = entered.get_future();
    auto block = release.get_future().share();
    std::atomic_int calls = 0;
    auto token = std::make_shared<int>(1);
    std::weak_ptr<int> retained = token;
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    workers.Launch([&] {
        dispatcher.ProcessJob();
    });
    dispatcher.Post(workQueue, false, [&] {
        ++calls;
        entered.set_value();
        block.wait();
    });
    const bool started = ready.wait_for(2s) == std::future_status::ready;
    dispatcher.Post(workQueue, false, [&, token] {
        ++calls;
    });
    token.reset();
    dispatcher.Stop();
    release.set_value();
    workers.StopAndJoin();
    dispatcher.Clear();
    workQueue.Clear();
    Check(started && calls == 1, "stop does not execute buffered user work");
    Check(retained.expired(), "stop releases buffered captures after workers join");
}
}

int RunLobbyDispatcherTests()
{
    ParallelAndSerial();
    MigrationOrder();
    GlobalBarrier();
    StopWithPending();

    return 207;
}
