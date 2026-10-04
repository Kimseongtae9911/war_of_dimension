#pragma once

namespace wod_server {
    struct DebuffInfo
    {
        EDebuffType debuffType = EDebuffType::None;
        float debuffValue = 0.f;
        uint16_t debuffDuration = 0;
        uint8_t debuffRepeatTime = 0;
        uint16_t debuffDistance = 0;
    };

    struct BuffInfo
    {
        EBuffType buffType = EBuffType::None;
        float buffValue = 0.f;
        uint16_t buffDuration = 0;
        uint8_t buffRepeatTime = 0;
        uint16_t buffDistance = 0;
    };

    struct SkillCsv
    {
        SkillCsv(const tabledata::SkillInfo& skillInfo) {
            type = skillInfo.Type;
            strengthRatio = skillInfo.StrengthRatio;
            magicRatio = skillInfo.MagicRatio;
            castingTime = skillInfo.CastingTime;
            posOffset = skillInfo.PosOffset;
            skillRadius = skillInfo.SkillRadius;
            damageCycleTime = skillInfo.DamageCycleTime;
            repeatTime = skillInfo.RepeatTime;
            speed = skillInfo.Speed;
            damageReduction = skillInfo.DamageReduction;
            extraParam1 = skillInfo.ExtraParam1;
            extraParam2 = skillInfo.ExtraParam2;
            extraParam3 = skillInfo.ExtraParam3;

            if (skillInfo.Extent != "NULL") {
                std::istringstream issExtent(skillInfo.Extent);
                std::string tokenExtent;
                std::getline(issExtent, tokenExtent, ';');
                extent.x = std::stof(tokenExtent);
                std::getline(issExtent, tokenExtent, ';');
                extent.y = std::stof(tokenExtent);
                std::getline(issExtent, tokenExtent, ';');
                extent.z = std::stof(tokenExtent);
            }

            if (skillInfo.DebuffType != "NULL") {
                std::istringstream issType(skillInfo.DebuffType);
                std::istringstream issValue(skillInfo.DebuffValue);
                std::istringstream issDuration(skillInfo.DebuffDuration);
                std::istringstream issRepeatTime(skillInfo.DebuffRepeatTime);
                std::istringstream issDistance(skillInfo.DebuffDistance);

                std::string tokenType, tokenValue, tokenDuration, tokenRepeatTime, tokenDistance;
                while (std::getline(issType, tokenType, ';') &&
                    std::getline(issValue, tokenValue, ';') &&
                    std::getline(issDuration, tokenDuration, ';') &&
                    std::getline(issRepeatTime, tokenRepeatTime, ';') &&
                    std::getline(issDistance, tokenDistance, ';')) {
                    DebuffInfo info;
                    info.debuffType = StringToEnum<EDebuffType>(tokenType);
                    info.debuffValue = std::stof(tokenValue);
                    info.debuffDuration = static_cast<uint16_t>(std::stoi(tokenDuration));
                    info.debuffRepeatTime = static_cast<uint8_t>(std::stoi(tokenRepeatTime));
                    info.debuffDistance = static_cast<uint16_t>(std::stoi(tokenDistance));
                    debuffInfo.emplace(info.debuffType, info);
                }
            }
            
            if (skillInfo.BuffType != "NULL") {
                std::istringstream issBuffType(skillInfo.BuffType);
                std::istringstream issBuffValue(skillInfo.BuffValue);
                std::istringstream issBuffDuration(skillInfo.BuffDuration);
                std::istringstream issBuffRepeatTime(skillInfo.BuffRepeatTime);
                std::istringstream issBuffDistance(skillInfo.BuffDistance);

                std::string tokenBuffType, tokenBuffValue, tokenBuffDuration, tokenBuffRepeatTime, tokenBuffDistance;
                while (std::getline(issBuffType, tokenBuffType, ';') &&
                    std::getline(issBuffValue, tokenBuffValue, ';') &&
                    std::getline(issBuffDuration, tokenBuffDuration, ';') &&
                    std::getline(issBuffRepeatTime, tokenBuffRepeatTime, ';') &&
                    std::getline(issBuffDistance, tokenBuffDistance, ';')) {
                    BuffInfo info;
                    info.buffType = StringToEnum<EBuffType>(tokenBuffType);
                    info.buffValue = std::stof(tokenBuffValue);
                    info.buffDuration = static_cast<uint16_t>(std::stoi(tokenBuffDuration));
                    info.buffRepeatTime = static_cast<uint8_t>(std::stoi(tokenBuffRepeatTime));
                    if (tokenBuffDistance != "NULL")
                        info.buffDistance = static_cast<uint16_t>(std::stoi(tokenBuffDistance));
                    buffInfo.emplace(info.buffType, info);
                }
            }
        }

        EPlayerSkill type = EPlayerSkill::None;
        float strengthRatio = 0.f;
        float magicRatio = 0.f;
        uint16_t castingTime = 0;
        float posOffset = 0.f;
        float skillRadius = 0.f;
        uint16_t damageCycleTime = 0;
        uint8_t repeatTime = 0;
        std::map<EDebuffType, DebuffInfo> debuffInfo;
        float speed = 0.f;
        std::map<EBuffType, BuffInfo> buffInfo;
        float damageReduction = 0.f;
        Vector3 extent;
        float extraParam1 = 0.f;
        float extraParam2 = 0.f;
        float extraParam3 = 0.f;

        const int16_t GetDamage(int16_t strength, int16_t magic) const {
			return static_cast<int16_t>(strength * strengthRatio + magic * magicRatio);
		}

        const vec3 GetStartPos(const vec3& pos, const vec3& look) const {
            return pos + look * posOffset;
        }
    };

    class SkillCsvMgr : public CsvLoader, public TSingleton<SkillCsvMgr>
    {
    public:
        bool Initialize() override;
        bool Release() override;

        void LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader) override;
        SkillCsv* GetSkillCsv(EPlayerSkill type) const {
            auto iter = m_skillCsvMap.find(type);
			if (iter == m_skillCsvMap.end()) {
				return nullptr;
			}
			return iter->second;
        }

    private:
        std::unordered_map<EPlayerSkill, SkillCsv*> m_skillCsvMap;
    };

}