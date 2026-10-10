#pragma once

namespace wod_server {
    struct DebuffInfo
    {
        EDebuffType m_debuffType = EDebuffType::None;
        float m_debuffValue = 0.f;
        uint16_t m_debuffDuration = 0;
        uint8_t m_debuffRepeatTime = 0;
        uint16_t m_debuffDistance = 0;
    };

    struct BuffInfo
    {
        EBuffType m_buffType = EBuffType::None;
        float m_buffValue = 0.f;
        uint16_t m_buffDuration = 0;
        uint8_t m_buffRepeatTime = 0;
        uint16_t m_buffDistance = 0;
    };

    struct SkillCsv
    {
        SkillCsv(const tabledata::SkillInfo& _skillInfo) {
            m_type = _skillInfo.Type;
            m_strengthRatio = _skillInfo.StrengthRatio;
            m_magicRatio = _skillInfo.MagicRatio;
            m_castingTime = _skillInfo.CastingTime;
            m_posOffset = _skillInfo.PosOffset;
            m_skillRadius = _skillInfo.SkillRadius;
            m_damageCycleTime = _skillInfo.DamageCycleTime;
            m_repeatTime = _skillInfo.RepeatTime;
            m_speed = _skillInfo.Speed;
            m_damageReduction = _skillInfo.DamageReduction;
            m_extraParam1 = _skillInfo.ExtraParam1;
            m_extraParam2 = _skillInfo.ExtraParam2;
            m_extraParam3 = _skillInfo.ExtraParam3;

            if (_skillInfo.Extent != "NULL") {
                std::istringstream issExtent(_skillInfo.Extent);
                std::string tokenExtent;
                std::getline(issExtent, tokenExtent, ';');
                m_extent.m_x = std::stof(tokenExtent);
                std::getline(issExtent, tokenExtent, ';');
                m_extent.m_y = std::stof(tokenExtent);
                std::getline(issExtent, tokenExtent, ';');
                m_extent.m_z = std::stof(tokenExtent);
            }

            if (_skillInfo.DebuffType != "NULL") {
                std::istringstream issType(_skillInfo.DebuffType);
                std::istringstream issValue(_skillInfo.DebuffValue);
                std::istringstream issDuration(_skillInfo.DebuffDuration);
                std::istringstream issRepeatTime(_skillInfo.DebuffRepeatTime);
                std::istringstream issDistance(_skillInfo.DebuffDistance);

                std::string tokenType, tokenValue, tokenDuration, tokenRepeatTime, tokenDistance;
                while (std::getline(issType, tokenType, ';') &&
                    std::getline(issValue, tokenValue, ';') &&
                    std::getline(issDuration, tokenDuration, ';') &&
                    std::getline(issRepeatTime, tokenRepeatTime, ';') &&
                    std::getline(issDistance, tokenDistance, ';')) {
                    DebuffInfo info;
                    info.m_debuffType = StringToEnum<EDebuffType>(tokenType);
                    info.m_debuffValue = std::stof(tokenValue);
                    info.m_debuffDuration = static_cast<uint16_t>(std::stoi(tokenDuration));
                    info.m_debuffRepeatTime = static_cast<uint8_t>(std::stoi(tokenRepeatTime));
                    info.m_debuffDistance = static_cast<uint16_t>(std::stoi(tokenDistance));
                    m_debuffInfo.emplace(info.m_debuffType, info);
                }
            }

            if (_skillInfo.BuffType != "NULL") {
                std::istringstream issBuffType(_skillInfo.BuffType);
                std::istringstream issBuffValue(_skillInfo.BuffValue);
                std::istringstream issBuffDuration(_skillInfo.BuffDuration);
                std::istringstream issBuffRepeatTime(_skillInfo.BuffRepeatTime);
                std::istringstream issBuffDistance(_skillInfo.BuffDistance);

                std::string tokenBuffType, tokenBuffValue, tokenBuffDuration, tokenBuffRepeatTime, tokenBuffDistance;
                while (std::getline(issBuffType, tokenBuffType, ';') &&
                    std::getline(issBuffValue, tokenBuffValue, ';') &&
                    std::getline(issBuffDuration, tokenBuffDuration, ';') &&
                    std::getline(issBuffRepeatTime, tokenBuffRepeatTime, ';') &&
                    std::getline(issBuffDistance, tokenBuffDistance, ';')) {
                    BuffInfo info;
                    info.m_buffType = StringToEnum<EBuffType>(tokenBuffType);
                    info.m_buffValue = std::stof(tokenBuffValue);
                    info.m_buffDuration = static_cast<uint16_t>(std::stoi(tokenBuffDuration));
                    info.m_buffRepeatTime = static_cast<uint8_t>(std::stoi(tokenBuffRepeatTime));
                    if (tokenBuffDistance != "NULL")
                        info.m_buffDistance = static_cast<uint16_t>(std::stoi(tokenBuffDistance));
                    m_buffInfo.emplace(info.m_buffType, info);
                }
            }
        }

        EPlayerSkill m_type = EPlayerSkill::None;
        float m_strengthRatio = 0.f;
        float m_magicRatio = 0.f;
        uint16_t m_castingTime = 0;
        float m_posOffset = 0.f;
        float m_skillRadius = 0.f;
        uint16_t m_damageCycleTime = 0;
        uint8_t m_repeatTime = 0;
        std::map<EDebuffType, DebuffInfo> m_debuffInfo;
        float m_speed = 0.f;
        std::map<EBuffType, BuffInfo> m_buffInfo;
        float m_damageReduction = 0.f;
        Vector3 m_extent;
        float m_extraParam1 = 0.f;
        float m_extraParam2 = 0.f;
        float m_extraParam3 = 0.f;

        const int16_t GetDamage(int16_t _strength, int16_t _magic) const {
			return static_cast<int16_t>(_strength * m_strengthRatio + _magic * m_magicRatio);
		}

        const vec3 GetStartPos(const vec3& _pos, const vec3& _look) const {
            return _pos + _look * m_posOffset;
        }
    };

    class SkillCsvMgr : public CsvLoader, public TSingleton<SkillCsvMgr>
    {
    public:
        bool Initialize() override;
        bool Release() override;

        void LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader) override;
        SkillCsv* GetSkillCsv(EPlayerSkill _type) const {
            auto iter = m_skillCsvMap.find(_type);
			if (iter == m_skillCsvMap.end()) {
				return nullptr;
			}
			return iter->second;
        }

    private:
        std::unordered_map<EPlayerSkill, SkillCsv*> m_skillCsvMap;
    };

}