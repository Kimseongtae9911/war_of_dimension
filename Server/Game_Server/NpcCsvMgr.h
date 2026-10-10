#pragma once
namespace wod_server {
    struct NpcCsv : public tabledata::NpcInfo
    {
        NpcCsv(const tabledata::NpcInfo& _npcInfo) {
            Type = _npcInfo.Type;
            RespawnTime = _npcInfo.RespawnTime;
            BaseHp = _npcInfo.BaseHp;
            HpIncrease = _npcInfo.HpIncrease;
            BaseAttack = _npcInfo.BaseAttack;
            AttackIncrease = _npcInfo.AttackIncrease;
            AttackDistance = _npcInfo.AttackDistance;
            AttackCooltime = _npcInfo.AttackCooltime;
            GoldReward = _npcInfo.GoldReward;
            HealCooltime = _npcInfo.HealCooltime;
            HealPercent = _npcInfo.HealPercent;
            Speed = _npcInfo.Speed;
            RotateSpeed = _npcInfo.RotateSpeed;
            ChaseDistance = _npcInfo.ChaseDistance;
            ChaseMaxDistance = _npcInfo.ChaseMaxDistance;
            Scale = _npcInfo.Scale;

            if (_npcInfo.BuffType != "NULL") {
                std::istringstream issBuffType(_npcInfo.BuffType);
                std::istringstream issBuffValue(_npcInfo.BuffValue);
                std::istringstream issBuffDuration(_npcInfo.BuffDuration);

                std::string tokenBuffType, tokenBuffValue, tokenBuffDuration;
                while (std::getline(issBuffType, tokenBuffType, ';') &&
                    std::getline(issBuffValue, tokenBuffValue, ';') &&
                    std::getline(issBuffDuration, tokenBuffDuration, ';')) {
                    BuffInfo info;
                    info.m_buffType = StringToEnum<EBuffType>(tokenBuffType);
                    info.m_buffValue = std::stof(tokenBuffValue);
                    info.m_buffDuration = static_cast<uint16_t>(std::stoi(tokenBuffDuration));
                    m_BuffInfos.emplace(info.m_buffType, info);
                }
            }

            if (_npcInfo.RespawnPos != "NULL") {
                int8_t cnt = 0;
                std::istringstream issRespawnPos(_npcInfo.RespawnPos);
                std::string tokenRespawnPos;
                vec3 tempPos;
                while (std::getline(issRespawnPos, tokenRespawnPos, ';')) {
                    if (cnt == 0)
                        tempPos.m_x = std::stof(tokenRespawnPos);
                    else if(cnt == 1)
                        tempPos.m_y = std::stof(tokenRespawnPos);
                    else
                        tempPos.m_z = std::stof(tokenRespawnPos);
                    if (++cnt % 3 == 0) {
                        m_respawnPos.push_back(tempPos);
                        cnt = 0;
                    }
                }
            }

            if (_npcInfo.RespawnLook != "NULL") {
                int8_t cnt = 0;
                std::istringstream issRespawnLook(_npcInfo.RespawnLook);
                std::string tokenRespawnLook;
                vec3 tempLook;
                while (std::getline(issRespawnLook, tokenRespawnLook, ';')) {
                    if (cnt == 0)
                        tempLook.m_x = std::stof(tokenRespawnLook);
                    else if (cnt == 1)
                        tempLook.m_y = std::stof(tokenRespawnLook);
                    else
                        tempLook.m_z = std::stof(tokenRespawnLook);
                    if (++cnt % 3 == 0) {
                        m_respawnLook.push_back(tempLook);
                        cnt = 0;
                    }
                }
            }
        }
        std::vector<vec3> m_respawnPos;
        std::vector<vec3> m_respawnLook;
        std::map<EBuffType, BuffInfo> m_BuffInfos;
    };

	class NpcCsvMgr : public CsvLoader, public TSingleton<NpcCsvMgr>
	{
    public:
        bool Initialize() override;
        bool Release() override;

        void LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader) override;
        NpcCsv* GetNpcCsv(ENpcType _type) const {
            auto iter = m_npcCsvMap.find(_type);
            if (iter == m_npcCsvMap.end()) {
                return nullptr;
            }
            return iter->second;
        }

    private:
        std::unordered_map<ENpcType, NpcCsv*> m_npcCsvMap;
	};

}