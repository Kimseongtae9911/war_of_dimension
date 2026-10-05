#include "stdafx.h"
#include "ParticleSelection.h"

bool IsCompleteSkillLoadout(const IngameSkillLoadout& loadout)
{
    for (int player = 0; player < INGAME_PLAYER; ++player)
    {
        const int job = loadout.jobs[player];
        if (player < 3 ? (job < 0 || job >= MAX_JOB) : (job != 0 && job != 1 && job != 4 && job != 5)) return false;
        for (int skill : loadout.skills[player])
            if (player < 3 ? (skill < 48 || skill > 95) : (skill < 21 || skill > 40)) return false;
    }
    return true;
}

ParticleSelection::ParticleSelection(const IngameSkillLoadout& loadout) : complete(IsCompleteSkillLoadout(loadout))
{
    if (!complete) { required.set(); return; } // 수신 미완료는 기존 전체 풀을 보존한다.
    auto add = [this](SKILL_TYPE type) { required.set(static_cast<size_t>(type)); };
    add(SKILL_TYPE::TOWERATTACK);
    for (int player = 0; player < INGAME_PLAYER; ++player)
    {
        if (player < 3)
        {
            if (loadout.jobs[player] == 0) add(SKILL_TYPE::ARCHER_ATTACK);
            if (loadout.jobs[player] == 3) add(SKILL_TYPE::WIZARD_ATTACK);
        }
        else if (loadout.jobs[player] == 1 || loadout.jobs[player] == 5) add(SKILL_TYPE::PRO_ATTACK);
        else add(SKILL_TYPE::OGRE_ATTACK);

        for (int skill : loadout.skills[player])
        {
            // CSkillHandlerFactory: 영웅 번호 48..95. 보스는 수동 선택 응답과 같은 21..40.
            if (player == 3)
            {
                if (skill == 30) add(SKILL_TYPE::OGRE_DIMENSION_CRUSH);
                if (skill == 37) add(SKILL_TYPE::PRO_RETURN_ZERO);
                if (skill == 38) add(SKILL_TYPE::PRO_SCL);
                continue;
            }
            switch (skill)
            {
            case 53: add(SKILL_TYPE::ARCHER_PENETRAITING_SHOT); break;
            case 54: add(SKILL_TYPE::ARCHER_STICKY_ARROW); break;
            case 57: add(SKILL_TYPE::ARCHER_ARROW_RAIN); break;
            case 58: add(SKILL_TYPE::ARCHER_PHOENIX_ARROW); break;
            case 59: add(SKILL_TYPE::ARCHER_STROM_ARROW); break;
            case 70: add(SKILL_TYPE::FIGHTER_FIREBALL); break;
            case 79: add(SKILL_TYPE::SWORDMAN_AURA_BLADE); break;
            case 81: add(SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD); break;
            case 89: add(SKILL_TYPE::WIZARD_MAGIC_MISSILE); break;
            case 90: add(SKILL_TYPE::WIZARD_ENERGY_BALL); break;
            case 92: add(SKILL_TYPE::WIZARD_DARKNESS_RAY); break;
            case 94:
                add(SKILL_TYPE::WIZARD_BIGBANG);
                add(SKILL_TYPE::WIZARD_BIGBANG_CONTINUE); // CBigBang에서 생성하는 후속 효과
                break;
            default: break; // 버프·이동 등 슬롯 효과는 기존 별도 선택 슬롯 경로가 생성한다.
            }
        }
    }
}

bool ParticleSelection::Includes(SKILL_TYPE type) const
{
    const auto index = static_cast<size_t>(type);
    return index < required.size() && required.test(index);
}
