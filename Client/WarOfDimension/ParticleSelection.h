#pragma once
#include <array>
#include <bitset>
#include "../../Server/Game_Server/protocol.h"

struct IngameSkillLoadout
{
    std::array<int, INGAME_PLAYER> jobs{};
    std::array<std::array<int, MAX_SKILL>, INGAME_PLAYER> skills{};
};

bool IsCompleteSkillLoadout(const IngameSkillLoadout& loadout);

// 스킬 번호는 SC_SKILL_SELECT의 클라이언트 표현이다. 가변 GPU 버퍼는 공유하지 않는다.
class ParticleSelection
{
public:
    explicit ParticleSelection(const IngameSkillLoadout& loadout);
    bool Includes(SKILL_TYPE type) const;
    bool Complete() const { return complete; }
private:
    std::bitset<static_cast<size_t>(SKILL_TYPE::TYPE_COUNT)> required;
    bool complete;
};
