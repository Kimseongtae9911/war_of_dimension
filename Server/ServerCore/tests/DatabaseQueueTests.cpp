#include <ServerCore/OrderedWorkQueue.h>
#include "../../Lobby_Server/LobbyDispatcher.h"
#include "../../Lobby_Server/AccountOwners.h"
#include "../../Lobby_Server/RewardJournal.h"
#include "../../Lobby_Server/PasswordHash.h"
#include <array>
#include <future>
#include <iostream>
#include <thread>
#include <vector>

namespace
{
using namespace std::chrono_literals;
using wod::core::OrderedWorkQueue;
using wod_server::LobbyDispatcher;

void Check(bool _value, const char* _message)
{
    if (!_value)
        throw std::runtime_error(_message);
}

void FifoAndDrain()
{
    OrderedWorkQueue queue(2048);
    std::vector<int> actual;
    std::array<std::thread, 4> producers;
    for (int producer = 0; producer < 4; ++producer)
        producers[producer] = std::thread([&, producer] {
            for (int i = 0; i < 100; ++i)
                queue.Submit([&, value = producer * 100 + i] {
                    actual.push_back(value);
                });
        });

    for (auto& producer : producers)
        producer.join();

    queue.StopAccepting();
    queue.Run();
    std::array<int, 4> next{};
    for (int value : actual)
        Check(value % 100 == next[value / 100]++, "per-producer FIFO order");

    Check(actual.size() == 400 && queue.Accepted() == queue.Completed(), "all accepted work drained");
    Check(!queue.Submit([] {
    }),
          "submit after stop rejected");
}

void CapacityAndFailure()
{
    OrderedWorkQueue queue(2);
    int completed = 0;
    Check(queue.Submit([] {
        throw std::runtime_error("injected SQL error");
    }),
          "accept failure task");
    Check(queue.Submit([&] {
        ++completed;
    }),
          "accept second task");
    Check(!queue.Submit([] {
    }),
          "capacity rejects without waiting");
    queue.StopAccepting();
    queue.Run();
    Check(completed == 1 && queue.Failed() == 1 && queue.Completed() == 2 && queue.Rejected() == 1, "failure does not strand queue");
}

void SessionContinuation(bool _early)
{
    LobbyDispatcher dispatcher(1);
    LobbyDispatcher::SessionWorkQueue session, other;
    session.SetChannel(0);
    other.SetChannel(0);
    OrderedWorkQueue database;
    std::promise<void> submitted, unrelated, continuation, following, release;
    auto gate = release.get_future().share();
    auto ready = submitted.get_future();
    auto independent = unrelated.get_future();
    auto applied = continuation.get_future();
    auto next = following.get_future();
    std::atomic_int state = 0;
    std::atomic_bool wrongOrder = false;
    std::thread dbWorker([&] {
        database.Run();
    });
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    workers.Launch([&] {
        dispatcher.ProcessJob();
    });
    dispatcher.Post(session, true, [&] {
        dispatcher.Suspend(session);
        database.Submit([&] {
            gate.wait();
            dispatcher.Resume(session, [&] {
                state = 1;
                continuation.set_value();
            });
        });
        submitted.set_value();
        if (_early)
        {
            release.set_value();
            // Resume가 현재 작업의 epilogue보다 빨리 도착하는 경합을 강제한다.
            while (!database.Completed())
                std::this_thread::yield();
        }
    });
    const bool started = ready.wait_for(2s) == std::future_status::ready;
    dispatcher.Post(session, false, [&] {
        wrongOrder = state != 1;
        following.set_value();
    });
    dispatcher.Post(other, false, [&] {
        unrelated.set_value();
    });
    const bool freeWorker = independent.wait_for(2s) == std::future_status::ready;
    const bool held = _early || next.wait_for(20ms) == std::future_status::timeout;
    if (!_early)
        release.set_value();

    const bool finished = applied.wait_for(2s) == std::future_status::ready && next.wait_for(2s) == std::future_status::ready;
    database.StopAccepting();
    dbWorker.join();
    workers.StopAndJoin();
    dispatcher.Clear();
    Check(started && freeWorker && held && finished && !wrongOrder, "DB completion precedes next session request; other session progresses");
}

void AccountOwnership()
{
    wod_server::AccountOwners owners;
    std::array<std::thread, 4> threads;
    std::array<int, 4> identities{};
    std::atomic_int winner = -1;
    std::atomic_int claimed = 0;
    for (int i = 0; i < 4; ++i)
        threads[i] = std::thread([&, i] {
            if (owners.Claim(i % 2 ? "ACCOUNT " : "account", &identities[i]))
            {
                winner = i;
                ++claimed;
            }
        });

    for (auto& thread : threads)
        thread.join();

    int newcomer = 0;
    owners.Release("account", &newcomer);
    Check(claimed == 1 && !owners.Claim("account", &newcomer), "one account owner; wrong owner cannot release");
    owners.Release("account", &identities[winner]);
    Check(owners.Claim("account", &newcomer), "account released for reconnect");
}

void BoundedSessionInbox()
{
    LobbyDispatcher dispatcher(1, 2, 5);
    LobbyDispatcher::SessionWorkQueue session;
    session.SetChannel(0);
    dispatcher.Suspend(session);
    std::vector<int> order;
    Check(dispatcher.Post(
              session, false,
              [&] {
                  order.push_back(1);
              },
              3),
          "first inbox admission");
    Check(!dispatcher.Post(
              session, false,
              [] {
              },
              3),
          "inbox byte limit");
    Check(dispatcher.Post(
              session, false,
              [&] {
                  order.push_back(2);
              },
              2),
          "exact byte limit accepted");
    Check(!dispatcher.Post(session, false,
                           [] {
                           }),
          "inbox job limit");
    std::promise<void> cleaned;
    auto done = cleaned.get_future();
    Check(dispatcher.PostCleanup(session,
                                 [&] {
                                     order.push_back(3);
                                     cleaned.set_value();
                                 }),
          "cleanup bypasses full inbox");
    Check(dispatcher.Resume(session,
                            [&] {
                                order.push_back(0);
                            }),
          "completion bypasses full inbox");
    wod::core::ThreadGroup workers(
        [&] {
            dispatcher.Stop();
        },
        [](std::exception_ptr) {
        });
    workers.Launch([&] {
        dispatcher.ProcessJob();
    });
    const bool finished = done.wait_for(2s) == std::future_status::ready;
    workers.StopAndJoin();
    dispatcher.Clear();
    Check(finished && order == std::vector<int>({0, 1, 2, 3}), "full inbox drains in order and cleanup completes");
}

void RewardRecoveryRecord()
{
    const auto directory = std::filesystem::temp_directory_path() / ("NewWod-reward-test-" + std::to_string(GetCurrentProcessId()) + "-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    wod_server::RewardJournal::Record record;
    {
        wod_server::RewardJournal journal(directory);
        Check(journal.Recover().empty(), "new reward journal empty");
        record = journal.Append("account", 17);
        Check(journal.Pending() == 1, "reward persisted before SQL");
        bool excluded = false;
        try
        {
            wod_server::RewardJournal duplicate(directory);
        }
        catch (const std::runtime_error&)
        {
            excluded = true;
        }
        Check(excluded, "only one recovery owner");
    }

    auto writing = directory / (record.m_operation + ".writing");
    std::filesystem::rename(directory / (record.m_operation + ".pending"), writing);
    {
        wod_server::RewardJournal journal(directory);
        const auto recovered = journal.Recover();
        Check(recovered.size() == 1 && recovered[0].m_operation == record.m_operation && recovered[0].m_account == "account" && recovered[0].m_delta == 17, "flushed record promoted after restart");
        journal.Complete(recovered[0]);
        Check(journal.Pending() == 0 && journal.Recover().empty(), "committed record removed");
    }

    const auto corrupt = directory / (std::string(32, 'a') + ".writing");
    {
        std::ofstream output(corrupt);
        output << "incomplete";
    }
    {
        wod_server::RewardJournal journal(directory);
        bool rejected = false;
        try
        {
            journal.Recover();
        }
        catch (const std::runtime_error&)
        {
            rejected = true;
        }
        Check(rejected && std::filesystem::exists(corrupt), "corrupt record fails closed and remains for inspection");
    }

    // 이 검사에서 직접 만든 경로와 파일만 비재귀적으로 정리한다.
    std::filesystem::remove(corrupt);
    std::filesystem::remove(directory / "owner.lock");
    std::filesystem::remove(directory);
}

void PasswordStorage()
{
    using wod_server::PasswordHash;
    const std::string reference = "pbkdf2-sha256$600000$000102030405060708090a0b0c0d0e0f$9e5fa624ce69b150f21106af114736bbc8e76f82893c562aa0321d325dc154cc";
    Check(PasswordHash::Verify("testpass", reference), "CNG matches independent PBKDF2 vector");
    Check(!PasswordHash::Verify("Testpass", reference), "password comparison is case sensitive");
    const auto a = PasswordHash::Encode("testpass");
    const auto b = PasswordHash::Encode("testpass");
    Check(a != b && PasswordHash::Verify("testpass", a), "independent salts and round trip");
    Check(!PasswordHash::Verify("testpass", "testpass") && !PasswordHash::Verify("testpass", "pbkdf2-sha256$1$bad"), "plaintext and malformed stored values rejected");
}

double Ms(std::chrono::steady_clock::time_point _start)
{
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - _start).count();
}
}

int RunDatabaseQueueTests()
{
    FifoAndDrain();
    CapacityAndFailure();
    SessionContinuation(false);
    SessionContinuation(true);
    AccountOwnership();
    BoundedSessionInbox();
    RewardRecoveryRecord();
    PasswordStorage();

    return 8;
}

int RunDatabaseBenchmark()
{
    // 기존 잠금 보유 경계와 현재 production 큐/dispatcher를 비교하는 제어 실험.
    // SQL 처리속도나 실제 서버 수용량을 측정하는 benchmark가 아니다.
    std::cout << "{\"schema_version\":1,\"kind\":\"controlled DB wait boundary\",\"jobs\":8,\"injected_delay_ms\":10,\"runs\":[";
    bool comma = false;
    for (int run = -1; run < 5; ++run)
    {
        for (int position = 0; position < 2; ++position)
        {
            const bool async = (run + position + 2) % 2 == 1;
            LobbyDispatcher dispatcher(1);
            LobbyDispatcher::SessionWorkQueue request, other;
            request.SetChannel(0);
            other.SetChannel(0);
            OrderedWorkQueue database;
            std::promise<void> entered, response, done;
            auto startSignal = entered.get_future();
            auto responded = response.get_future();
            auto drained = done.get_future();
            double callerMs = 0;
            const auto start = std::chrono::steady_clock::now();
            std::thread db([&] {
                database.Run();
            });
            wod::core::ThreadGroup workers(
                [&] {
                    dispatcher.Stop();
                },
                [](std::exception_ptr) {
                });
            workers.Launch([&] {
                dispatcher.ProcessJob();
            });
            dispatcher.Post(request, true, [&] {
                const auto callStart = std::chrono::steady_clock::now();
                const auto sql = [&] {
                    entered.set_value();
                    for (int i = 0; i < 8; ++i)
                        std::this_thread::sleep_for(10ms);
                };
                if (async)
                {
                    dispatcher.Suspend(request);
                    database.Submit([&, sql] {
                        sql();
                        dispatcher.Resume(request, [&] {
                            done.set_value();
                        });
                    });
                }
                else
                {
                    sql();
                    done.set_value();
                }

                callerMs = Ms(callStart);
            });
            Check(startSignal.wait_for(3s) == std::future_status::ready, "benchmark started");
            const auto responseStart = std::chrono::steady_clock::now();
            dispatcher.Post(other, false, [&] {
                response.set_value();
            });
            Check(responded.wait_for(3s) == std::future_status::ready, "other session completed");
            const auto responseMs = Ms(responseStart);
            Check(drained.wait_for(3s) == std::future_status::ready, "DB work completed");
            database.StopAccepting();
            db.join();
            workers.StopAndJoin();
            dispatcher.Clear();
            if (run < 0)
                continue;

            if (comma)
                std::cout << ',';

            comma = true;
            std::cout << "{\"run\":" << run << ",\"async\":" << (async ? "true" : "false") << ",\"caller_block_ms\":" << callerMs << ",\"other_session_ms\":" << responseMs << ",\"total_ms\":" << Ms(start) << '}';
        }
    }

    std::cout << "]}\n";

    return 0;
}
