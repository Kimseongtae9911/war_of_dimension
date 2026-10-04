#pragma once
namespace wod_server {
    struct NpcCsv : public tabledata::NpcInfo
    {
        NpcCsv(const tabledata::NpcInfo& npcInfo) {
            Type = npcInfo.Type;
            RespawnTime = npcInfo.RespawnTime;
            BaseHp = npcInfo.BaseHp;
            HpIncrease = npcInfo.HpIncrease;
            BaseAttack = npcInfo.BaseAttack;
            AttackIncrease = npcInfo.AttackIncrease;
            AttackDistance = npcInfo.AttackDistance;
            AttackCooltime = npcInfo.AttackCooltime;
            GoldReward = npcInfo.GoldReward;
            HealCooltime = npcInfo.HealCooltime;
            HealPercent = npcInfo.HealPercent;
            Speed = npcInfo.Speed;
            RotateSpeed = npcInfo.RotateSpeed;
            ChaseDistance = npcInfo.ChaseDistance;
            ChaseMaxDistance = npcInfo.ChaseMaxDistance;
            Scale = npcInfo.Scale;

            if (npcInfo.BuffType != "NULL") {
                std::istringstream issBuffType(npcInfo.BuffType);
                std::istringstream issBuffValue(npcInfo.BuffValue);
                std::istringstream issBuffDuration(npcInfo.BuffDuration);

                std::string tokenBuffType, tokenBuffValue, tokenBuffDuration;
                while (std::getline(issBuffType, tokenBuffType, ';') &&
                    std::getline(issBuffValue, tokenBuffValue, ';') &&
                    std::getline(issBuffDuration, tokenBuffDuration, ';')) {
                    BuffInfo info;
                    info.buffType = StringToEnum<EBuffType>(tokenBuffType);
                    info.buffValue = std::stof(tokenBuffValue);
                    info.buffDuration = static_cast<uint16_t>(std::stoi(tokenBuffDuration));
                    BuffInfos.emplace(info.buffType, info);
                }
            }

            if (npcInfo.RespawnPos != "NULL") {
                int8_t cnt = 0;
                std::istringstream issRespawnPos(npcInfo.RespawnPos);
                std::string tokenRespawnPos;
                vec3 tempPos;
                while (std::getline(issRespawnPos, tokenRespawnPos, ';')) {
                    if (cnt == 0)
                        tempPos.x = std::stof(tokenRespawnPos);
                    else if(cnt == 1)
                        tempPos.y = std::stof(tokenRespawnPos);
                    else
                        tempPos.z = std::stof(tokenRespawnPos);
                    if (++cnt % 3 == 0) {
                        respawnPos.push_back(tempPos);
                        cnt = 0;
                    }
                }
            }

            if (npcInfo.RespawnLook != "NULL") {
                int8_t cnt = 0;
                std::istringstream issRespawnLook(npcInfo.RespawnLook);
                std::string tokenRespawnLook;
                vec3 tempLook;
                while (std::getline(issRespawnLook, tokenRespawnLook, ';')) {
                    if (cnt == 0)
                        tempLook.x = std::stof(tokenRespawnLook);
                    else if (cnt == 1)
                        tempLook.y = std::stof(tokenRespawnLook);
                    else
                        tempLook.z = std::stof(tokenRespawnLook);
                    if (++cnt % 3 == 0) {
                        respawnLook.push_back(tempLook);
                        cnt = 0;
                    }
                }
            }
        }
        std::vector<vec3> respawnPos;
        std::vector<vec3> respawnLook;
        std::map<EBuffType, BuffInfo> BuffInfos;
    };

	class NpcCsvMgr : public CsvLoader, public TSingleton<NpcCsvMgr>
	{
    public:
        bool Initialize() override;
        bool Release() override;

        void LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader) override;
        NpcCsv* GetNpcCsv(ENpcType type) const {
            auto iter = m_npcCsvMap.find(type);
            if (iter == m_npcCsvMap.end()) {
                return nullptr;
            }
            return iter->second;
        }

    private:
        std::unordered_map<ENpcType, NpcCsv*> m_npcCsvMap;
	};

}