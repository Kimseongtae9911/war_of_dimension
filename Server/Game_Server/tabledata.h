#pragma once

namespace tabledata {

struct ItemInfo {
    EItemType Type = EItemType::None;
    float Value = 0.f;
    uint32_t Time = 0;
};

struct NpcInfo {
    ENpcType Type = ENpcType::None;
    std::string RespawnPos;
    std::string ExtraPos;
    std::string RespawnLook;
    std::string ExtraLook;
    uint32_t RespawnTime = 0;
    float BaseHp = 0.f;
    float HpIncrease = 0.f;
    uint16_t BaseAttack = 0;
    uint8_t AttackIncrease = 0;
    float AttackDistance = 0.f;
    uint16_t AttackCooltime = 0;
    uint16_t GoldReward = 0;
    std::string BuffType;
    std::string BuffValue;
    std::string BuffDuration;
    uint16_t HealCooltime = 0;
    float HealPercent = 0.f;
    float MaxSpeed = 0.f;
    float Speed = 0.f;
    uint16_t RotateSpeed = 0;
    float ChaseDistance = 0.f;
    float ChaseMaxDistance = 0.f;
    float Scale = 0.f;
};

struct SkillInfo {
    EPlayerSkill Type = EPlayerSkill::None;
    float StrengthRatio = 0.f;
    float MagicRatio = 0.f;
    uint16_t CastingTime = 0;
    float PosOffset = 0.f;
    float SkillRadius = 0.f;
    uint16_t DamageCycleTime = 0;
    uint8_t RepeatTime = 0;
    std::string DebuffType;
    std::string DebuffValue;
    std::string DebuffDuration;
    std::string DebuffRepeatTime;
    std::string DebuffDistance;
    float Speed = 0.f;
    std::string BuffType;
    std::string BuffValue;
    std::string BuffDuration;
    std::string BuffRepeatTime;
    std::string BuffDistance;
    float DamageReduction = 0.f;
    std::string Extent;
    float ExtraParam1 = 0.f;
    float ExtraParam2 = 0.f;
    float ExtraParam3 = 0.f;
};

} // namespace tabledata
