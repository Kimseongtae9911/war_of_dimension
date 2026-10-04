#pragma once

namespace tabledata {

struct ItemInfo {
    EItemType Type = EItemType::None;
    float Value = 0.f;
    uint32 Time;
};

struct NpcInfo {
    ENpcType Type = ENpcType::None;
    FString RespawnPos;
    FString ExtraPos;
    FString RespawnLook;
    FString ExtraLook;
    uint32 RespawnTime;
    float BaseHp = 0.f;
    float HpIncrease = 0.f;
    uint16 BaseAttack;
    uint8 AttackIncrease;
    float AttackDistance = 0.f;
    uint16 AttackCooltime;
    uint16 GoldReward;
    FString BuffType;
    FString BuffValue;
    FString BuffDuration;
    uint16 HealCooltime;
    float HealPercent = 0.f;
    float MaxSpeed = 0.f;
    float Speed = 0.f;
    uint16 RotateSpeed;
    float ChaseDistance = 0.f;
    float ChaseMaxDistance = 0.f;
    float Scale = 0.f;
};

struct SkillInfo {
    EPlayerSkill Type = EPlayerSkill::None;
    float StrengthRatio = 0.f;
    float MagicRatio = 0.f;
    uint16 CastingTime;
    float PosOffset = 0.f;
    float SkillRadius = 0.f;
    uint16 DamageCycleTime;
    uint8 RepeatTime;
    FString DebuffType;
    FString DebuffValue;
    FString DebuffDuration;
    FString DebuffRepeatTime;
    FString DebuffDistance;
    float Speed = 0.f;
    FString BuffType;
    FString BuffValue;
    FString BuffDuration;
    FString BuffRepeatTime;
    FString BuffDistance;
    float DamageReduction = 0.f;
    FString Extent;
    float ExtraParam1 = 0.f;
    float ExtraParam2 = 0.f;
    float ExtraParam3 = 0.f;
};

} // namespace tabledata
