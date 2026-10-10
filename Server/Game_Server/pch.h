#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>
#undef GetObject
#include <thread>
#include <mutex>
#include <shared_mutex>
#include <vector>
#include <chrono>
#include <cstdint>
#include <Protocol/protocol.h>

#include <iostream>
#include <ranges>
#include <string>
#include <fstream>
#include <memory>
#include <atomic>
#include <array>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <stack>
#include <tuple>
#include <utility>
#include <typeinfo>
#include <type_traits>
#include <concurrent_priority_queue.h>
#include <concurrent_unordered_map.h>

#include <functional>

#include <DirectXCollision.h>
#include <DirectXMath.h>

#include <rapidjson/document.h>
#include <rapidjson/writer.h>
#include <rapidjson/stringbuffer.h>

#include <boost/pfr.hpp>

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

class Session;
using CSessionPool = wod::core::SessionPool<Session>;
using Resource = wod::core::SessionResources<OverlapEx, Session>;
}

#include "Global.h"

#include "CommonConstants.h"
#include "Interface.h"
#include "tableEnum.h"
#include "tabledata.h"

#include "MathUtil.h"
#include "RandomUtil.h"
#include "TimeUtil.h"
#include "GameUtil.h"
#include "CsvLoader.h"
#include "SkillCsvMgr.h"
#include "NpcCsvMgr.h"
#include "ItemCsvMgr.h"
#include "SkillTimerDefine.h"

#include "GameObject.h"
#include "CClient.h"
#include "CMatch.h"

#include "CItemFactory.h"

#include "CGameMgr.h"
#include "CObjectMgr.h"
#include "CMatchMgr.h"
#include "CNetworkMgr.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

//Game Constants
constexpr int TELEPORT_COOLTIME = 30000;
constexpr std::array<vec2, 2> TELEPORT_TARGET_POS = { vec2(-19.96f, -106.97f), vec2(-102.19f, -25.83f) };
constexpr std::array<vec2, 2> TELEPORT_POS = { vec2(-97.5f, -30.1f), vec2(-24.5f, -103.5f) };
constexpr float TELEPORT_SPEED = 40.0f;

constexpr std::array<vec2, 6> JUMP_START_POS = { vec2(-7.46f, -79.55f), vec2(-40.3f, -48.82f), vec2(-66.6f, -9.7f), vec2(-113.7f, -52.6f), vec2(-79.69f, -80.74f),vec2(-46.88f, -118.05f) };
constexpr std::array<vec2, 6> JUMP_LANDING_POS = { vec2(-8.7f, -88.32f), vec2(-34.49f, -42.63f), vec2(-77.35f, -10.64f), vec2(-110.93f, -43.38f), vec2(-86.26f, -86.77f),vec2(-38.43f, -114.7f) };
constexpr std::array<vec3, 6> JUMP_CONTROL_POS = { vec3(-8.3f, 40.f, -84.f), vec3(-37.281f, 40.f, -44.822f), vec3(-72.4f, 40.f, -11.1f), vec3(-112.f, 40.f, -49.3f), vec3(-83.7f, 40.f, -83.9f), vec3(-42.5f, 40.f, -116.3f) };
constexpr float JUMP_TIME = 1.0f;

constexpr vec3 TOWER_POS[PATH_NUM] = { vec3(-68.47f, 9.17f, -134.98f), vec3(-78.08f, 9.17f, -103.54f), vec3(-102.89f, 9.17f, -78.76f), vec3(-129.93f, 9.17f, -70.61f) };
constexpr int TOWER_INIT_POWER = 80;
constexpr int TOWER_POWER_INCREASE = 30;

#define SKILL_ADDITIONAL_SPEED_FROM_STAT(stat) (stat - 1.0f) * 5.0f