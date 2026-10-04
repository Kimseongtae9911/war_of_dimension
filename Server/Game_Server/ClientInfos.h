#pragma once
#include <array>
#include "MathUtil.h"

namespace ClientInfos {
	static constexpr std::array<vec3, 3> HERO_START_POS = { vec3(-134.f, 5.21f, -139.f), vec3(-138.f, 5.21f, -138.f), vec3(-136.f, 5.21f, -142.f) };
	static constexpr vec3 BOSS_START_POS = { 18.f, 5.21f, 6.2f };

	static constexpr int BOSS_BASE_HEAL_AMOUNT = 800;
	static constexpr int HERO_BASE_HEAL_AMOUNT = 300;

	static constexpr int HERO_INIT_RESPAWN_TIME = 10000;

	static constexpr int HEAL_COOLTIME = 8000;
	static constexpr int BASE_HEAL_COOLTIME = 1000;

	static constexpr int HERO_KILL_GOLD = 700;

	static constexpr vec3 HERO_ATTACK_OFFSET = { 0.f, 0.9f, 0.f };
	static constexpr vec3 OGRE_ATTACK_OFFSET = { 0.0f, 1.2f, 0.0f };
}