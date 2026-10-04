#pragma once
constexpr int LOBBY_SERVER_ID = 9999;

constexpr int MAX_ROLE = 6;
constexpr int NPC_ID = 2000;

constexpr int WORLD_WIDTH = 300;
constexpr int WORLD_HEIGHT = 300;
constexpr int SECTION_NUM = 10;
constexpr int MAX_MATCH = 250;
constexpr int MAX_PLAYER = 4;
constexpr int MAX_CLIENT = MAX_MATCH * MAX_PLAYER;
constexpr int SKILL_OFFSET = 116;

constexpr int MAX_SOCKET = MAX_MATCH * MAX_PLAYER * 2;
constexpr int MAX_OVEREX_OBJECT = MAX_MATCH * MAX_PLAYER * 2;

constexpr float TOWER_ATTACK_DISTANCE = 8.f;
constexpr float NEXUS_ATTACK_DISTANCE = 7.f;
constexpr float MOVETO_STRUCTURE_DISTANCE = 15.f;