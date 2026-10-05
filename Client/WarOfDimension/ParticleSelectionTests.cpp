#include "stdafx.h"
#include "ParticleSelection.h"
#include "NetworkManager.h"
#include <filesystem>
#include <stdexcept>

namespace PARTICLE_SKILLSETTING { int NumParticle(SKILL_TYPE); }
namespace
{
    void Require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
    IngameSkillLoadout Fixture()
    {
        return {{0, 1, 2, 4}, {{{48, 53, 54, 58}, {60, 65, 69, 70}, {72, 79, 81, 82}, {24, 27, 29, 30}}}};
    }
}

int RunParticleSelectionTestCases(const wchar_t* reportPath)
{
    std::ofstream report{std::filesystem::path(reportPath)};
    if (!report) return 1;
    try
    {
        auto fixture = Fixture();
        const ParticleSelection selection(fixture);
        Require(selection.Complete(), "valid loadout rejected");
        int full = 0, selected = 0;
        for (int i = 0; i < static_cast<int>(SKILL_TYPE::TYPE_COUNT); ++i)
        {
            const auto type = static_cast<SKILL_TYPE>(i);
            full += PARTICLE_SKILLSETTING::NumParticle(type) * MAX_SKILL_OBJECT;
            if (selection.Includes(type)) selected += PARTICLE_SKILLSETTING::NumParticle(type) * MAX_SKILL_OBJECT;
        }
        Require(full == 100 && selected == 45, "fixture pool count changed");
        Require(selection.Includes(SKILL_TYPE::FIGHTER_FIREBALL) && selection.Includes(SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD), "other player dependency missing");
        Require(selection.Includes(SKILL_TYPE::TOWERATTACK) && selection.Includes(SKILL_TYPE::ARCHER_ATTACK), "mandatory dependency missing");
        Require(!selection.Includes(SKILL_TYPE::WIZARD_ATTACK) && !selection.Includes(SKILL_TYPE::WIZARD_BIGBANG), "unselected dependency retained");
        fixture.jobs[2] = 3; fixture.skills[2] = {89, 90, 92, 94};
        const ParticleSelection wizard(fixture);
        for (auto type : {SKILL_TYPE::WIZARD_ATTACK, SKILL_TYPE::WIZARD_MAGIC_MISSILE, SKILL_TYPE::WIZARD_ENERGY_BALL,
                          SKILL_TYPE::WIZARD_DARKNESS_RAY, SKILL_TYPE::WIZARD_BIGBANG, SKILL_TYPE::WIZARD_BIGBANG_CONTINUE})
            Require(wizard.Includes(type), "wizard/follow-up dependency missing");
        fixture.jobs[3] = 1; fixture.skills[3] = {31, 37, 38, 40};
        const ParticleSelection pro(fixture);
        for (auto type : {SKILL_TYPE::PRO_ATTACK, SKILL_TYPE::PRO_RETURN_ZERO, SKILL_TYPE::PRO_SCL})
            Require(pro.Includes(type), "programmer dependency missing");
        fixture.skills[0] = {53, 53, 53, 53};
        const ParticleSelection duplicates(fixture);
        Require(duplicates.Includes(SKILL_TYPE::ARCHER_PENETRAITING_SHOT) && !duplicates.Includes(SKILL_TYPE::ARCHER_STICKY_ARROW), "duplicate union incorrect");
        fixture.skills[0] = {48, 57, 58, 59};
        Require(ParticleSelection(fixture).Includes(SKILL_TYPE::ARCHER_ARROW_RAIN) && ParticleSelection(fixture).Includes(SKILL_TYPE::ARCHER_STROM_ARROW), "timer dependency missing");
        int rejected = 0;
        for (int player = 0; player < INGAME_PLAYER; ++player)
            for (int slot = 0; slot < MAX_SKILL; ++slot)
            {
                auto bad = Fixture(); bad.skills[player][slot] = 0;
                const ParticleSelection fallback(bad);
                Require(!fallback.Complete(), "incomplete selection accepted");
                for (int i = 0; i < static_cast<int>(SKILL_TYPE::TYPE_COUNT); ++i)
                    Require(fallback.Includes(static_cast<SKILL_TYPE>(i)), "fallback omitted effect");
                ++rejected;
            }
        auto bad = Fixture(); bad.jobs[0] = -1;
        Require(!ParticleSelection(bad).Complete() && !selection.Includes(static_cast<SKILL_TYPE>(-1)), "invalid range accepted");
        auto network = NetworkManager::GetInstance(); network->Initialize(""); network->Reset();
        network->FreezeIngameSkills(); Require(!network->GetFrozenIngameSkills(), "incomplete snapshot frozen");
        const auto seed = Fixture();
        for (int player = 0; player < INGAME_PLAYER; ++player)
        {
            network->StoreReadyJob(player, seed.jobs[player]);
            for (int slot = 0; slot < MAX_SKILL; ++slot) network->StoreReadySkill(player, slot, seed.skills[player][slot]);
        }
        network->StoreReadySkill(-1, 0, 94); network->StoreReadySkill(0, 4, 94); network->StoreReadyJob(4, 3);
        network->FreezeIngameSkills();
        Require(network->GetFrozenIngameSkills().has_value(), "complete snapshot missing");
        network->StoreReadySkill(0, 0, 94); network->StoreReadyJob(0, 3);
        const auto frozen = network->GetIngameSkillLoadout();
        Require(frozen.skills == seed.skills && frozen.jobs == seed.jobs, "frozen loadout mutated");
        network->Reset(); Require(!network->GetFrozenIngameSkills() && !IsCompleteSkillLoadout(network->GetIngameSkillLoadout()), "match reset failed");
        report << "{\"ok\":true,\"fullPoolCount\":" << full << ",\"selectedPoolCount\":" << selected
            << ",\"incompleteSlotCases\":" << rejected << ",\"snapshotChecks\":true,\"followUpAndTimerChecks\":true}\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        OutputDebugStringA(error.what()); report << "{\"ok\":false}\n"; return 1;
    }
}
