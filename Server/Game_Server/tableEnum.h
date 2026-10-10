#pragma once

template<class T>
T StringToEnum(const std::string& _str)
{
    return static_cast<T>(std::stoi(_str));
}

enum class EItemType : uint8_t
{
    None = 0,
    HealHp = 1,
    HealMp = 2,
    MaxHp = 3,
    MaxMp = 4,
    StrengthIncrease = 5,
    MagicIncrease = 6,
    DefenseIncrease = 7,
    RegistIncrease = 8,
    SpeedIncrease = 9,
    TenacityIncrease = 10,
    CriticalIncrease = 11,
    Max = 12
};

template<>
EItemType StringToEnum(const std::string& _str) {
    static std::unordered_map<std::string, EItemType> enumMap = {
    {"HealHp", EItemType::HealHp}, {"HealMp", EItemType::HealMp}, {"MaxHp", EItemType::MaxHp}, {"MaxMp", EItemType::MaxMp}, {"StrengthIncrease", EItemType::StrengthIncrease}, {"MagicIncrease", EItemType::MagicIncrease}, {"DefenseIncrease", EItemType::DefenseIncrease}, {"RegistIncrease", EItemType::RegistIncrease}, {"SpeedIncrease", EItemType::SpeedIncrease}, {"TenacityIncrease", EItemType::TenacityIncrease}, {"CriticalIncrease", EItemType::CriticalIncrease},     };
    auto it = enumMap.find(_str);
    if (it != enumMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid enum string");
    }
}

enum class ENpcType : uint16_t
{
    None = 0,
    Minion = 1,
    RedDragon = 2,
    GreenDragon = 3,
    Golem = 4,
    Bear = 5,
    Minotaur = 6,
    Chest = 7,
    Beholder = 8,
    Max = 9
};

template<>
ENpcType StringToEnum(const std::string& _str) {
    static std::unordered_map<std::string, ENpcType> enumMap = {
    {"Minion", ENpcType::Minion}, {"RedDragon", ENpcType::RedDragon}, {"GreenDragon", ENpcType::GreenDragon}, {"Golem", ENpcType::Golem}, {"Bear", ENpcType::Bear}, {"Minotaur", ENpcType::Minotaur}, {"Chest", ENpcType::Chest}, {"Beholder", ENpcType::Beholder},     };
    auto it = enumMap.find(_str);
    if (it != enumMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid enum string");
    }
}

enum class EPlayerSkill : uint16_t
{
    None = 0,
    ArcherAttack = 1,
    ArcherBackStep = 2,
    ArcherDodge = 3,
    ArcherMultipleShot = 4,
    ArcherVault = 5,
    ArcherVitalPoint = 6,
    ArcherPenetraitingShot = 7,
    ArcherStickyArrow = 8,
    ArcherHunterEyes = 9,
    ArcherWindStep = 10,
    ArcherArrowRain = 11,
    ArcherPhoenixArrow = 12,
    ArcherStormArrow = 13,
    FighterAttack = 14,
    FighterDash = 15,
    FighterDodge = 16,
    FighterSpinKick = 17,
    FighterWildAttack = 18,
    FighterDragonFist = 19,
    FighterMeditation = 20,
    FighterPointBlood = 21,
    FighterIndestructible = 22,
    FighterWindKick = 23,
    FighterCounter = 24,
    FighterFireBall = 25,
    FighterRisingDragon = 26,
    SwordManAttack = 27,
    SwordManDodge = 28,
    SwordManRun = 29,
    SwordManHeavySlash = 30,
    SwordManShieldBash = 31,
    SwordManWarCry = 32,
    SwordManDefensiveStance = 33,
    SwordManBerserk = 34,
    SwordManAuraBlade = 35,
    SwordManHellBlade = 36,
    SwordManJudgementSword = 37,
    SwordManProtectedArea = 38,
    SwordManAnkleCut = 39,
    WizardAttack = 40,
    WizardTeleport = 41,
    WizardBlink = 42,
    WizardBodyStrength = 43,
    WizardEnchant = 44,
    WizardEarthImpact = 45,
    WizardMagicMissile = 46,
    WizardEnergyBall = 47,
    WizardMagicEye = 48,
    WizardDarknessRay = 49,
    WizardReflect = 50,
    WizardBigBang = 51,
    WizardBigBangContinue = 52,
    WizardOverload = 53,
    OgreAttack = 54,
    OgreHeavySwing = 55,
    OgreCrunch = 56,
    OgreCharging = 57,
    OgreRoar = 58,
    OgreGluttony = 59,
    OgreEndure = 60,
    OgreRockThrow = 61,
    OgreDimensionPunch = 62,
    OgreButting = 63,
    OgreDimensionCrush = 64,
    ProgrammerAttack = 65,
    ProgrammerMove = 66,
    ProgrammerPointer = 67,
    ProgrammerPlusStats = 68,
    ProgrammerMinusStats = 69,
    ProgrammerRelease = 70,
    ProgrammerDelete = 71,
    ProgrammerReturn0 = 72,
    ProgrammerLaser = 73,
    ProgrammerWhileTrue = 74,
    ProgrammerHelloWorld = 75,
    Burn = 76,
    Stun = 77,
    MemoryLeak = 78,
    Silence = 79,
    Max = 80
};

template<>
EPlayerSkill StringToEnum(const std::string& _str) {
    static std::unordered_map<std::string, EPlayerSkill> enumMap = {
    {"ArcherAttack", EPlayerSkill::ArcherAttack}, {"ArcherBackStep", EPlayerSkill::ArcherBackStep}, {"ArcherDodge", EPlayerSkill::ArcherDodge}, {"ArcherMultipleShot", EPlayerSkill::ArcherMultipleShot}, {"ArcherVault", EPlayerSkill::ArcherVault}, {"ArcherVitalPoint", EPlayerSkill::ArcherVitalPoint}, {"ArcherPenetraitingShot", EPlayerSkill::ArcherPenetraitingShot}, {"ArcherStickyArrow", EPlayerSkill::ArcherStickyArrow}, {"ArcherHunterEyes", EPlayerSkill::ArcherHunterEyes}, {"ArcherWindStep", EPlayerSkill::ArcherWindStep}, {"ArcherArrowRain", EPlayerSkill::ArcherArrowRain}, {"ArcherPhoenixArrow", EPlayerSkill::ArcherPhoenixArrow}, {"ArcherStormArrow", EPlayerSkill::ArcherStormArrow}, {"FighterAttack", EPlayerSkill::FighterAttack}, {"FighterDash", EPlayerSkill::FighterDash}, {"FighterDodge", EPlayerSkill::FighterDodge}, {"FighterSpinKick", EPlayerSkill::FighterSpinKick}, {"FighterWildAttack", EPlayerSkill::FighterWildAttack}, {"FighterDragonFist", EPlayerSkill::FighterDragonFist}, {"FighterMeditation", EPlayerSkill::FighterMeditation}, {"FighterPointBlood", EPlayerSkill::FighterPointBlood}, {"FighterIndestructible", EPlayerSkill::FighterIndestructible}, {"FighterWindKick", EPlayerSkill::FighterWindKick}, {"FighterCounter", EPlayerSkill::FighterCounter}, {"FighterFireBall", EPlayerSkill::FighterFireBall}, {"FighterRisingDragon", EPlayerSkill::FighterRisingDragon}, {"SwordManAttack", EPlayerSkill::SwordManAttack}, {"SwordManDodge", EPlayerSkill::SwordManDodge}, {"SwordManRun", EPlayerSkill::SwordManRun}, {"SwordManHeavySlash", EPlayerSkill::SwordManHeavySlash}, {"SwordManShieldBash", EPlayerSkill::SwordManShieldBash}, {"SwordManWarCry", EPlayerSkill::SwordManWarCry}, {"SwordManDefensiveStance", EPlayerSkill::SwordManDefensiveStance}, {"SwordManBerserk", EPlayerSkill::SwordManBerserk}, {"SwordManAuraBlade", EPlayerSkill::SwordManAuraBlade}, {"SwordManHellBlade", EPlayerSkill::SwordManHellBlade}, {"SwordManJudgementSword", EPlayerSkill::SwordManJudgementSword}, {"SwordManProtectedArea", EPlayerSkill::SwordManProtectedArea}, {"SwordManAnkleCut", EPlayerSkill::SwordManAnkleCut}, {"WizardAttack", EPlayerSkill::WizardAttack}, {"WizardTeleport", EPlayerSkill::WizardTeleport}, {"WizardBlink", EPlayerSkill::WizardBlink}, {"WizardBodyStrength", EPlayerSkill::WizardBodyStrength}, {"WizardEnchant", EPlayerSkill::WizardEnchant}, {"WizardEarthImpact", EPlayerSkill::WizardEarthImpact}, {"WizardMagicMissile", EPlayerSkill::WizardMagicMissile}, {"WizardEnergyBall", EPlayerSkill::WizardEnergyBall}, {"WizardMagicEye", EPlayerSkill::WizardMagicEye}, {"WizardDarknessRay", EPlayerSkill::WizardDarknessRay}, {"WizardReflect", EPlayerSkill::WizardReflect}, {"WizardBigBang", EPlayerSkill::WizardBigBang}, {"WizardBigBangContinue", EPlayerSkill::WizardBigBangContinue}, {"WizardOverload", EPlayerSkill::WizardOverload}, {"OgreAttack", EPlayerSkill::OgreAttack}, {"OgreHeavySwing", EPlayerSkill::OgreHeavySwing}, {"OgreCrunch", EPlayerSkill::OgreCrunch}, {"OgreCharging", EPlayerSkill::OgreCharging}, {"OgreRoar", EPlayerSkill::OgreRoar}, {"OgreGluttony", EPlayerSkill::OgreGluttony}, {"OgreEndure", EPlayerSkill::OgreEndure}, {"OgreRockThrow", EPlayerSkill::OgreRockThrow}, {"OgreDimensionPunch", EPlayerSkill::OgreDimensionPunch}, {"OgreButting", EPlayerSkill::OgreButting}, {"OgreDimensionCrush", EPlayerSkill::OgreDimensionCrush}, {"ProgrammerAttack", EPlayerSkill::ProgrammerAttack}, {"ProgrammerMove", EPlayerSkill::ProgrammerMove}, {"ProgrammerPointer", EPlayerSkill::ProgrammerPointer}, {"ProgrammerPlusStats", EPlayerSkill::ProgrammerPlusStats}, {"ProgrammerMinusStats", EPlayerSkill::ProgrammerMinusStats}, {"ProgrammerRelease", EPlayerSkill::ProgrammerRelease}, {"ProgrammerDelete", EPlayerSkill::ProgrammerDelete}, {"ProgrammerReturn0", EPlayerSkill::ProgrammerReturn0}, {"ProgrammerLaser", EPlayerSkill::ProgrammerLaser}, {"ProgrammerWhileTrue", EPlayerSkill::ProgrammerWhileTrue}, {"ProgrammerHelloWorld", EPlayerSkill::ProgrammerHelloWorld}, {"Burn", EPlayerSkill::Burn}, {"Stun", EPlayerSkill::Stun}, {"MemoryLeak", EPlayerSkill::MemoryLeak}, {"Silence", EPlayerSkill::Silence},     };
    auto it = enumMap.find(_str);
    if (it != enumMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid enum string");
    }
}

enum class EDebuffType : uint8_t
{
    None = 0,
    Slow = 1,
    Burn = 2,
    Silence = 3,
    Stun = 4,
    Poison = 5,
    MemoryLeak = 6,
    ArmorDecrease = 7,
    UtilDecrease = 8,
    MaxHpDecreasePercent = 9,
    Bleed = 10,
    StatDecrease = 11,
    Max = 80
};

template<>
EDebuffType StringToEnum(const std::string& _str) {
    static std::unordered_map<std::string, EDebuffType> enumMap = {
    {"Slow", EDebuffType::Slow}, {"Burn", EDebuffType::Burn}, {"Silence", EDebuffType::Silence}, {"Stun", EDebuffType::Stun}, {"Poison", EDebuffType::Poison}, {"MemoryLeak", EDebuffType::MemoryLeak}, {"ArmorDecrease", EDebuffType::ArmorDecrease}, {"UtilDecrease", EDebuffType::UtilDecrease}, {"MaxHpDecreasePercent", EDebuffType::MaxHpDecreasePercent}, {"Bleed", EDebuffType::Bleed}, {"StatDecrease", EDebuffType::StatDecrease},     };
    auto it = enumMap.find(_str);
    if (it != enumMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid enum string");
    }
}

enum class EBuffType : uint8_t
{
    None = 0,
    CriticalIncrease = 1,
    SpeedIncrease = 2,
    AttackIncrease = 3,
    UtilIncrease = 4,
    Reflect = 5,
    DefenseIncrease = 6,
    StrengthIncrease = 7,
    Cooltime = 8,
    StatIncrease = 9,
    Max = 80
};

template<>
EBuffType StringToEnum(const std::string& _str) {
    static std::unordered_map<std::string, EBuffType> enumMap = {
    {"CriticalIncrease", EBuffType::CriticalIncrease}, {"SpeedIncrease", EBuffType::SpeedIncrease}, {"AttackIncrease", EBuffType::AttackIncrease}, {"UtilIncrease", EBuffType::UtilIncrease}, {"Reflect", EBuffType::Reflect}, {"DefenseIncrease", EBuffType::DefenseIncrease}, {"StrengthIncrease", EBuffType::StrengthIncrease}, {"Cooltime", EBuffType::Cooltime}, {"StatIncrease", EBuffType::StatIncrease},     };
    auto it = enumMap.find(_str);
    if (it != enumMap.end()) {
        return it->second;
    } else {
        throw std::invalid_argument("Invalid enum string");
    }
}

