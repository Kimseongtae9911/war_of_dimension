#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <WS2tcpip.h>
#include <MSWSock.h>
#include <thread>
#include <mutex>
#include <shared_mutex>

#include <iostream>
#include <string>
#include <memory>
#include <fstream>
#include <chrono>
#include <array>
#include <queue>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_map.h>
#include <functional>
#include <iomanip>
#include <ctime>
#include <cstdlib>

#include <Protocol/protocol.h>

constexpr int MAX_CLIENT = 3000;

inline int ReadLobbySetting(const char* _name, int _fallback, int _maximum)
{
#ifdef LOCAL_TEST
    char text[32]{};
    const auto length = GetEnvironmentVariableA(_name, text, sizeof(text));
    if (length)
    {
        if (length >= sizeof(text))
            throw std::invalid_argument(_name);

        char* end = nullptr;
        const long value = std::strtol(text, &end, 10);
        if (end == text || *end != '\0' || value < 1 || value > _maximum)
            throw std::invalid_argument(_name);

        return static_cast<int>(value);
    }
#endif

    return _fallback;
}

inline const int LOBBY_CHANNEL_CAPACITY = [] {
#ifdef LOCAL_TEST
    char value[2]{};
    const bool uncapped = GetEnvironmentVariableA("WOD_PERF_UNCAPPED", value, sizeof(value)) == 1 && value[0] == '1';
    return ReadLobbySetting("WOD_LOBBY_CHANNEL_CAPACITY", uncapped ? MAX_CLIENT : LOBBY_MAX_CLIENT, MAX_CLIENT);
#else
    return LOBBY_MAX_CLIENT;
#endif
}();
inline const int CHANNEL_NUM = ReadLobbySetting("WOD_LOBBY_CHANNELS", MAX_CLIENT / LOBBY_CHANNEL_CAPACITY, MAX_CLIENT / LOBBY_CHANNEL_CAPACITY);
inline const int LOBBY_WORKERS = ReadLobbySetting("WOD_LOBBY_WORKERS", 4, 32);

#include <ServerCore/Net.h>
#include <ServerCore/Concurrency.h>
#include <ServerCore/Diagnostics.h>
#include <ServerCore/Resource.h>
#include "enum.h"

using IJob = wod::core::IJob;
template <class Func> using Job = wod::core::Job<Func>;

namespace wod_server
{
using wod::core::SockAddr;
using wod::core::NetworkRuntime;
using wod::core::LogPrinter;
using IJobQueue = wod::core::JobQueue;
using JobQueue = wod::core::JobQueue;
using OverlapEx = wod::core::TaggedIoContext<OP_TYPE>;
using SocketResource = wod::core::SocketResource;
using CSocketPool = wod::core::SocketPool;
using Resource = wod::core::SocketResources<OverlapEx>;
}

#include "Global.h"

#include <DirectXCollision.h>
#include <DirectXMath.h>
#include "../Game_Server/Interface.h"
#include "../Game_Server/MathUtil.h"
#include "GameUtil.h"

#include "CClient.h"
#include "CUserMgr.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

constexpr int MATCH_PLAYER = 4;

constexpr int VIEW_DISTANCE = 30;

constexpr const int SHOP_BUY_TOKEN_NUM = 100;
constexpr const int MAX_SOCKET = MAX_CLIENT * 2;
constexpr const int MAX_OVEREX_OBJECT = MAX_CLIENT * 2;
constexpr const char* GAME_IP = "222.99.104.16";

constexpr const int MIN_STAKE_DAYS = 100;
constexpr const int MIN_STAKE_TOKEN = 10;

static int GenerateRandomNumber(int _min, int _max)
{
    std::random_device rd;
    std::mt19937 engine(rd());

    std::uniform_int_distribution<int> distribution(_min, _max);

    return distribution(engine);
}
