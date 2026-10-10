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

#include <Protocol/protocol.h>

constexpr int MAX_CLIENT = 3000;
constexpr int CHANNEL_NUM = (MAX_CLIENT / LOBBY_MAX_CLIENT);

#include "Global.h"

#include "enum.h"
#include "OverlapEx.h"
#include "LogUtil.h"
#include <DirectXCollision.h>
#include <DirectXMath.h>
#include "../Game_Server/Interface.h"
#include "../Game_Server/MathUtil.h"
#include "GameUtil.h"

#include "CClient.h"
#include "CUserMgr.h"
#include "Job.h"
#include "JobQueue.h"

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

static int GenerateRandomNumber(int _min, int _max) {
	std::random_device rd;
	std::mt19937 engine(rd());

	std::uniform_int_distribution<int> distribution(_min, _max);

	return distribution(engine);
}