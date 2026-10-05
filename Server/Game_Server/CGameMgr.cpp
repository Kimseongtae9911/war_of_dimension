#include "pch.h"
#include "CClient.h"
#include "CSkill.h"
#include "ClientInfos.h"

constexpr int BOSS_SKILL_NUM = 10;
constexpr int PLAYER_SKILL_NUM = 12;

namespace wod_server {
    std::unique_ptr<CGameMgr> CGameMgr::m_instance;

    bool CGameMgr::Initialize()
    {
        for (int i = 0; i < m_pathNums.size(); ++i) {
            m_pathNums[i] = 3;
        }

        for (int i = 0; i < m_gameData.size(); ++i) {
            m_gameData[i] = new GameData();
        }

        for (int i = 0; i < m_towers.size(); ++i) {
            for (int j = 0; j < m_towers[i].size(); ++j) {
                m_towers[i][j] = new CTower(i, j);
                m_towers[i][j]->SetPos(TOWER_POS[j]);
            }
        }
        
        for (int i = 0; i < m_towerAttack.size(); ++i) {
            for (int j = 0; j < m_towerAttack[i].size(); ++j) {
                m_towerAttack[i][j] = new CTowerAttack(); 
                m_towerAttack[i][j]->SetMatchNum(i);
                m_towerAttack[i][j]->SetID(j);
            }
        }

        for (int i = 0; i < m_nexus.size(); ++i) {
            m_nexus[i] = new CNexus(i);
        }

        for (int i = 0; i < MAX_MATCH; ++i) {
            for (int j = 0; j < MAX_SKILL_OBJECT; ++j) {
                m_wizardAttacks[i][j] = new CWizardAttack();
                m_wizardAttacks[i][j]->SetMatchNum(i);
                m_wizardAttacks[i][j]->SetID(j);
                m_wizardAttacks[i][j]->SetObjectType(SKILL_TYPE::WIZARD_ATTACK);

                m_wizardMissiles[i][j] = new CMagicMissile();
                m_wizardMissiles[i][j]->SetMatchNum(i);
                m_wizardMissiles[i][j]->SetID(j);
                m_wizardMissiles[i][j]->SetObjectType(SKILL_TYPE::WIZARD_MAGIC_MISSILE);

                m_wizardMagicEyes[i][j] = new CMagicEye();
                m_wizardMagicEyes[i][j]->SetMatchNum(i);
                m_wizardMagicEyes[i][j]->SetID(j);
                m_wizardMagicEyes[i][j]->SetObjectType(SKILL_TYPE::WIZARD_MAGIC_EYE);

                m_wizardEnergyBalls[i][j] = new CEnergyBall();
                m_wizardEnergyBalls[i][j]->SetMatchNum(i);
                m_wizardEnergyBalls[i][j]->SetID(j);
                m_wizardEnergyBalls[i][j]->SetObjectType(SKILL_TYPE::WIZARD_ENERGY_BALL);

                m_wizardBigBang[i][j] = new CBigBang();
                m_wizardBigBang[i][j]->SetMatchNum(i);
                m_wizardBigBang[i][j]->SetID(j);
                m_wizardBigBang[i][j]->SetObjectType(SKILL_TYPE::WIZARD_BIGBANG);

                m_swordManAuraBlade[i][j] = new CAuraBlade();
                m_swordManAuraBlade[i][j]->SetMatchNum(i);
                m_swordManAuraBlade[i][j]->SetID(j);
                m_swordManAuraBlade[i][j]->SetObjectType(SKILL_TYPE::SWORDMAN_AURA_BLADE);

                m_swordManJudgementSword[i][j] = new CJudgementSword();
                m_swordManJudgementSword[i][j]->SetMatchNum(i);
                m_swordManJudgementSword[i][j]->SetID(j);
                m_swordManJudgementSword[i][j]->SetObjectType(SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD);

                m_swordManProtectedArea[i][j] = new CProtectedArea();
                m_swordManProtectedArea[i][j]->SetMatchNum(i);
                m_swordManProtectedArea[i][j]->SetID(j);
                m_swordManProtectedArea[i][j]->SetObjectType(SKILL_TYPE::SWORDMAN_PROTECTED_AREA);

                m_archerAttacks[i][j] = new CArcherAttack();
                m_archerAttacks[i][j]->SetMatchNum(i);
                m_archerAttacks[i][j]->SetID(j);
                m_archerAttacks[i][j]->SetObjectType(SKILL_TYPE::ARCHER_ATTACK);

                m_archerPhoenixArrows[i][j] = new CPhoenixArrow();
                m_archerPhoenixArrows[i][j]->SetMatchNum(i);
                m_archerPhoenixArrows[i][j]->SetID(j);
                m_archerPhoenixArrows[i][j]->SetObjectType(SKILL_TYPE::ARCHER_PHOENIX_ARROW);

                m_archerStickyArrows[i][j] = new CStickyArrow();
                m_archerStickyArrows[i][j]->SetMatchNum(i);
                m_archerStickyArrows[i][j]->SetID(j);
                m_archerStickyArrows[i][j]->SetObjectType(SKILL_TYPE::ARCHER_STICKY_ARROW);

                m_archerPenetraitingShot[i][j] = new CPenetraitingShot();
                m_archerPenetraitingShot[i][j]->SetMatchNum(i);
                m_archerPenetraitingShot[i][j]->SetID(j);
                m_archerPenetraitingShot[i][j]->SetObjectType(SKILL_TYPE::ARCHER_PENETRAITING_SHOT);

                m_figtherFireBall[i][j] = new CFireBall();
                m_figtherFireBall[i][j]->SetMatchNum(i);
                m_figtherFireBall[i][j]->SetID(j);
                m_figtherFireBall[i][j]->SetObjectType(SKILL_TYPE::FIGHTER_FIREBALL);

                m_ogreRockThrow[i][j] = new CRockThrow();
                m_ogreRockThrow[i][j]->SetMatchNum(i);
                m_ogreRockThrow[i][j]->SetID(j);
                m_ogreRockThrow[i][j]->SetObjectType(SKILL_TYPE::OGRE_ROCK_THROW);

                m_ogreDimensionCrush[i][j] = new CDimensionCrush();
                m_ogreDimensionCrush[i][j]->SetMatchNum(i);
                m_ogreDimensionCrush[i][j]->SetID(j);
                m_ogreDimensionCrush[i][j]->SetObjectType(SKILL_TYPE::OGRE_DIMENSION_CRUSH);

                m_proReturnZero[i][j] = new CReturnZero();
                m_proReturnZero[i][j]->SetMatchNum(i);
                m_proReturnZero[i][j]->SetID(j);
                m_proReturnZero[i][j]->SetObjectType(SKILL_TYPE::PRO_RETURN_ZERO);

                m_proAttacks[i][j] = new CProAttack();
                m_proAttacks[i][j]->SetMatchNum(i);
                m_proAttacks[i][j]->SetID(j);
                m_proAttacks[i][j]->SetObjectType(SKILL_TYPE::PRO_ATTACK);

                m_proHelloWorld[i][j] = new CHelloWorld();
                m_proHelloWorld[i][j]->SetMatchNum(i);
                m_proHelloWorld[i][j]->SetID(j);
                m_proHelloWorld[i][j]->SetObjectType(SKILL_TYPE::PRO_HELLO_WORLD);
            }
            m_proWhileTrue[i] = false;
            m_ogreCharging[i] = new CCharging();
            m_ogreCharging[i]->SetMatchNum(i);
        }

        for (int i = 0; i < MAX_MATCH; ++i) {
            for (int j = 0; j < MAX_SKILL_OBJECT * 5; ++j) {
                m_archerMultipleShot[i][j] = new CMultipleShot();
                m_archerMultipleShot[i][j]->SetMatchNum(i);
                m_archerMultipleShot[i][j]->SetID(j);
                m_archerMultipleShot[i][j]->SetObjectType(SKILL_TYPE::ARCHER_MULTIPLE_SHOT);
            }
        }

        return true;
    }

    bool CGameMgr::Release()
    {
        for (int i = 0; i < m_gameData.size(); ++i) {
            if (m_gameData[i])
                delete m_gameData[i];
        }

        for (int i = 0; i < m_towers.size(); ++i) {
            for (int j = 0; j < m_towers[i].size(); ++j) {
                delete m_towers[i][j];
            }
        }

        for (int i = 0; i < m_towerAttack.size(); ++i) {
            for (int j = 0; j < m_towerAttack[i].size(); ++j) {
                delete m_towerAttack[i][j];
            }
        }

        for (int i = 0; i < m_nexus.size(); ++i) {
            delete m_nexus[i];
        }

        for (int i = 0; i < MAX_MATCH; ++i) {
            for (int j = 0; j < MAX_SKILL_OBJECT; ++j) {
                delete m_archerAttacks[i][j];
                delete m_wizardAttacks[i][j];
                delete m_wizardMissiles[i][j];
                delete m_wizardMagicEyes[i][j];
                delete m_wizardEnergyBalls[i][j];
                delete m_wizardBigBang[i][j];
                delete m_swordManAuraBlade[i][j];
                delete m_swordManJudgementSword[i][j];
                delete m_archerPhoenixArrows[i][j];
                delete m_archerStickyArrows[i][j];
                delete m_archerPenetraitingShot[i][j];
                delete m_figtherFireBall[i][j];
                delete m_ogreRockThrow[i][j];
                delete m_proReturnZero[i][j];
                delete m_proAttacks[i][j];
                delete m_proHelloWorld[i][j];
                delete m_ogreDimensionCrush[i][j];
            }
            delete m_ogreCharging[i];
        }

        for (int i = 0; i < MAX_MATCH; ++i) {
            for (int j = 0; j < MAX_SKILL_OBJECT * 5; ++j) {
                delete m_archerMultipleShot[i][j];
            }
        }

        return true;
    }

    void CGameMgr::Reset(int match)
    {
        m_gameData[match]->Reset();

        for (int i = 0; i < m_shopStatLevel[0].size(); ++i) {
            m_shopStatLevel[match][i].currentPrice = 10;
            m_shopStatLevel[match][i].level = 0;
        }

        for (int i = 0; i < PATH_NUM; ++i) {
            m_towers[match][i]->Reset();
            m_towerAttack[match][i]->active = false;
        }
        m_nexus[match]->Reset();

        //Skill
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            m_wizardAttacks[match][i]->active = false;
            m_wizardMissiles[match][i]->active = false;
            m_wizardMagicEyes[match][i]->active = false;
            m_wizardEnergyBalls[match][i]->active = false;
            m_wizardBigBang[match][i]->active = false;
            m_swordManAuraBlade[match][i]->active = false;
            m_swordManJudgementSword[match][i]->active = false;
            m_swordManProtectedArea[match][i]->active = false;
            m_archerAttacks[match][i]->active = false;
            m_archerStickyArrows[match][i]->active = false;
            m_archerPhoenixArrows[match][i]->active = false;
            m_archerPenetraitingShot[match][i]->active = false;
            m_figtherFireBall[match][i]->active = false;
            m_ogreRockThrow[match][i]->active = false;
            m_ogreDimensionCrush[match][i]->active = false;
            m_proReturnZero[match][i]->active = false;
            m_proHelloWorld[match][i]->active = false;
            m_proAttacks[match][i]->active = false;
        }
        for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
            m_archerMultipleShot[match][i]->active = false;
        }
        m_ogreCharging[match]->active = false;
        m_proWhileTrue[match] = false;
        m_skillMutex[match].lock();
        m_activeSkills[match].clear();
        m_skillMutex[match].unlock();
    }

    float CGameMgr::UpdateGameData(int match)
    {
        //Time Update
        float elapsedTime = TimeUtil::CalElapsedTime(m_gameData[match]->lastTime);
        m_gameData[match]->lastTime = TimeUtil::CurTime();
        m_gameData[match]->gameTime += elapsedTime;

        //Tower Update
        for (int i = 0; i < m_towers[match].size(); ++i) {
            m_towers[match][i]->Update(match);
        }
        for (int i = 0; i < m_towerAttack[match].size(); ++i) {
            m_towerAttack[match][i]->Update(elapsedTime);
        }

        std::vector<CGameObject*> activeSkills;
        std::vector<CGameObject*> updatedSkills;
        m_skillMutex[match].lock();
        activeSkills = m_activeSkills[match];
        m_skillMutex[match].unlock();
        for (CGameObject* skill : activeSkills)
        {
            if (skill->Update(elapsedTime))
                updatedSkills.push_back(skill);
        }

        {
            std::lock_guard<std::mutex> lock(m_skillMutex[match]);
            m_activeSkills[match] = std::move(updatedSkills);
        }

        //Send Time
        for (int i = 0; i < MAX_PLAYER; ++i) {
            if (CMatchMgr::GetInstance()->GetMatchPlayers(match)[i] == -1)
                continue;
            CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[i])->GetPacketSender()->SendGameTimePacket(static_cast<int>(::ceil(m_gameData[match]->gameTime)), 1);
        }

        //Check Fence Activation
        if (m_gameData[match]->fence && IsFloatEqual(::floor(m_gameData[match]->gameTime), 180.f)) {
            m_gameData[match]->fence = false;
        }

        return elapsedTime;
    }

    int CGameMgr::GetHeroRespawnTime(int match)
    {
        int respawnTime = ClientInfos::HERO_INIT_RESPAWN_TIME; // add time from calculation by game time

        return respawnTime;
    }

    void CGameMgr::SkillAutoSelect(int match)
    {
        int playerSkill = PLAYER_SKILL / 4;
        int bossSkill = BOSS_SKILL / 2;
        for (int i = 0; i < MAX_PLAYER; ++i) {
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[i]);

            const int bossJob = client->GetPlayerJob() >= MAX_JOB ? client->GetPlayerJob() - MAX_JOB : client->GetPlayerJob();
            for (int j = 1; j < MAX_SKILL + 1; ++j) {               
                if (client->GetSkillNum(j) == 0) {
                    if (j == MAX_SKILL) { //Ultimate
                        if (i == 3) {
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM);

                            client->SetSkillNum(j, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM - 1 - (21 - 96 - 1));
                        }
                        else {
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, playerSkill + client->GetPlayerJob() * PLAYER_SKILL_NUM + PLAYER_SKILL_NUM - 1);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, playerSkill + client->GetPlayerJob() * PLAYER_SKILL_NUM + PLAYER_SKILL_NUM - 1);

                            client->SetSkillNum(j, playerSkill + client->GetPlayerJob() * PLAYER_SKILL_NUM + PLAYER_SKILL_NUM - 1);
                        }
                    }
                    else {
                        if (i == 3) {
                            int skill = bossSkill + bossJob * BOSS_SKILL_NUM;
                            for (int k = 1; k < MAX_SKILL + 1; ++k) {
                                if (client->GetSkillNum(k) == skill) {
                                    skill++;
                                    k = 0;
                                }
                            }
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill + 1);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill + 1);
                            client->SetSkillNum(j, skill - (21 - 96 - 1));
                        }
                        else {
                            int skill = playerSkill + client->GetPlayerJob() * PLAYER_SKILL_NUM;
                            for (int k = 1; k < MAX_SKILL + 1; ++k) {
                                if (client->GetSkillNum(k) == skill) {
                                    skill++;
                                    k = 0;
                                }
                            }
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill);
                           client->SetSkillNum(j, skill);
                        }
                    }
                }
            }
        }
    }

    bool CGameMgr::CheckCoolTime(std::shared_ptr<CClient> client, char type)
    {
        if (client->GetUsingSkill()) {
            std::cout << "Using Skill" << std::endl;
            return false;
        }

        int skillCoolTime = client->GetSkillCoolTime(static_cast<int>(type) - 1);
        if (client->GetStatus()->coolTimeBuff == COOLTIME_BUFF::OVERLOAD)
            skillCoolTime = static_cast<int>(skillCoolTime * 0.5f);
        if (!client->GetUsingSkill() && std::chrono::duration_cast<std::chrono::seconds>(TimeUtil::CurTime() - client->GetSkillLastUsedTime(static_cast<int>(type) - 1)).count() >= skillCoolTime) {
            return true;
        }
        std::cout << "Skill CoolTime" << std::endl;
        return false;
    }

    void CGameMgr::ActiveTower(bool active, int match, int index)
    {
        m_towers[match][index]->active = active;
    }

    void CGameMgr::TowerAttack(int match, int targetID, const vec3& pos)
    {
        for (int i = 0; i < m_towerAttack[match].size(); ++i) {
            if (!m_towerAttack[match][i]->active) {
                m_towerAttack[match][i]->SetTarget(targetID);
                m_towerAttack[match][i]->SetPos(pos);
                m_towerAttack[match][i]->active = true;

                auto clients = CMatchMgr::GetInstance()->GetMatchPlayers(match);
                for (int j = 0; j < MAX_PLAYER; ++j) {
                    if (-1 == clients[j])
                        continue;
                    CObjectMgr::GetInstance()->GetClient(clients[j])->GetPacketSender()->SendTowerAttackAddPacket(i, pos);
                }

                break;
            }
        }
    }

    int CGameMgr::WizardAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardAttacks[matchNum][i]->active) {
                m_wizardAttacks[matchNum][i]->active = true;
                m_wizardAttacks[matchNum][i]->SetPos(pos);
                m_wizardAttacks[matchNum][i]->SetLook(look);
                m_wizardAttacks[matchNum][i]->SetPower(power);
                m_wizardAttacks[matchNum][i]->SetCritical(critical);
                m_wizardAttacks[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_wizardAttacks[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::MagicMissle(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardMissiles[matchNum][i]->active) {
                m_wizardMissiles[matchNum][i]->active = true;
                m_wizardMissiles[matchNum][i]->SetPos(pos);
                m_wizardMissiles[matchNum][i]->SetLook(look);
                m_wizardMissiles[matchNum][i]->SetPower(power);
                m_wizardMissiles[matchNum][i]->SetCritical(critical);
                m_wizardMissiles[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_wizardMissiles[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::MagicEye(int matchNum, const vec3& pos, const vec3& look)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardMagicEyes[matchNum][i]->active) {
                m_wizardMagicEyes[matchNum][i]->active = true;
                m_wizardMagicEyes[matchNum][i]->SetPos(pos);
                m_wizardMagicEyes[matchNum][i]->SetLook(look);
                m_wizardMagicEyes[matchNum][i]->ResetCheckTime();
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_wizardMagicEyes[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::MagicEye(int matchNum, int id)
    {
        if (m_wizardMagicEyes[matchNum][id]->active) {
            m_wizardMagicEyes[matchNum][id]->active = false;

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
                if (-1 == id)
                    continue;

                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(id, SKILL_TYPE::WIZARD_MAGIC_EYE);
            }
        }
    }

    int CGameMgr::EnergyBall(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardEnergyBalls[matchNum][i]->active) {
                m_wizardEnergyBalls[matchNum][i]->active = true;
                m_wizardEnergyBalls[matchNum][i]->SetPos(pos);
                m_wizardEnergyBalls[matchNum][i]->SetLook(look);
                m_wizardEnergyBalls[matchNum][i]->SetPower(power);
                m_wizardEnergyBalls[matchNum][i]->SetCritical(critical);
                m_wizardEnergyBalls[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_wizardEnergyBalls[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::BigBang(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardBigBang[matchNum][i]->active) {
                m_wizardBigBang[matchNum][i]->active = true;
                m_wizardBigBang[matchNum][i]->SetPos(pos);
                m_wizardBigBang[matchNum][i]->SetLook(look);
                m_wizardBigBang[matchNum][i]->SetPower(power);
                m_wizardBigBang[matchNum][i]->SetCritical(critical);
                m_wizardBigBang[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_wizardBigBang[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }
    
    int CGameMgr::AuraBlade(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManAuraBlade[matchNum][i]->active) {
                m_swordManAuraBlade[matchNum][i]->active = true;
                m_swordManAuraBlade[matchNum][i]->SetPos(pos);
                m_swordManAuraBlade[matchNum][i]->SetLook(look);
                m_swordManAuraBlade[matchNum][i]->SetPower(power);
                m_swordManAuraBlade[matchNum][i]->SetCritical(critical);
                m_swordManAuraBlade[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_swordManAuraBlade[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::JudgeMentSword(int matchNum, const vec3& pos, int power, int critical, int target, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManJudgementSword[matchNum][i]->active) {
                m_swordManJudgementSword[matchNum][i]->active = true;
                m_swordManJudgementSword[matchNum][i]->SetPos(pos);
                m_swordManJudgementSword[matchNum][i]->SetPower(power);
                m_swordManJudgementSword[matchNum][i]->SetTargetID(target);
                m_swordManJudgementSword[matchNum][i]->SetCritical(critical);
                m_swordManJudgementSword[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_swordManJudgementSword[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::ProtectedArea(int matchNum, const vec3& pos, const int power, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManProtectedArea[matchNum][i]->active) {
                m_swordManProtectedArea[matchNum][i]->active = true;
                m_swordManProtectedArea[matchNum][i]->SetPos(pos);
                m_swordManProtectedArea[matchNum][i]->SetDefensePower(power);
                m_swordManProtectedArea[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_swordManProtectedArea[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::ProtectedArea(int matchNum, int objectID)
    {
        if (m_swordManProtectedArea[matchNum][objectID]->active) {                       
            m_swordManProtectedArea[matchNum][objectID]->active = false;
            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
                if (id == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_PROTECTED_AREA);
            }
        }
    }

    int CGameMgr::ArcherAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerAttacks[matchNum][i]->active) {
                m_archerAttacks[matchNum][i]->active = true;
                m_archerAttacks[matchNum][i]->SetPos(pos);
                m_archerAttacks[matchNum][i]->SetLook(look);
                m_archerAttacks[matchNum][i]->SetPower(power);
                m_archerAttacks[matchNum][i]->SetCritical(critical);
                m_archerAttacks[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_archerAttacks[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::StickyArrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerStickyArrows[matchNum][i]->active) {
                m_archerStickyArrows[matchNum][i]->active = true;
                m_archerStickyArrows[matchNum][i]->SetPos(pos);
                m_archerStickyArrows[matchNum][i]->SetLook(look);
                m_archerStickyArrows[matchNum][i]->SetPower(power);
                m_archerStickyArrows[matchNum][i]->SetCritical(critical);
                m_archerStickyArrows[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_archerStickyArrows[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::PhoenixArrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerPhoenixArrows[matchNum][i]->active) {
                m_archerPhoenixArrows[matchNum][i]->active = true;
                m_archerPhoenixArrows[matchNum][i]->SetPos(pos);
                m_archerPhoenixArrows[matchNum][i]->SetLook(look);
                m_archerPhoenixArrows[matchNum][i]->SetPower(power);
                m_archerPhoenixArrows[matchNum][i]->SetCritical(critical);
                m_archerPhoenixArrows[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_archerPhoenixArrows[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::PenetraitingShot(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerPenetraitingShot[matchNum][i]->active) {
                m_archerPenetraitingShot[matchNum][i]->active = true;
                m_archerPenetraitingShot[matchNum][i]->SetPos(pos);
                m_archerPenetraitingShot[matchNum][i]->SetLook(look);
                m_archerPenetraitingShot[matchNum][i]->SetPower(power);
                m_archerPenetraitingShot[matchNum][i]->SetCritical(critical);
                m_archerPenetraitingShot[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_archerPenetraitingShot[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::ArcherMultipleShot(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        float angleIncrement = 20.0f;
        int numArrows = 5;

        float startAngle = -angleIncrement * (numArrows / 2);
        int count = 0;

        for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
            float angle = startAngle + angleIncrement * count;
            float radians = angle * (DirectX::XM_PI / 180.0f);
            vec3 rotatedLook = vec3(look.x * cos(radians) + look.z * sin(radians), look.y, look.z * cos(radians) - look.x * sin(radians));

            if (!m_archerMultipleShot[matchNum][i]->active) {
                m_archerMultipleShot[matchNum][i]->active = true;
                m_archerMultipleShot[matchNum][i]->SetPos(pos);
                m_archerMultipleShot[matchNum][i]->SetLook(vec3::Normalize(rotatedLook));
                m_archerMultipleShot[matchNum][i]->SetPower(power);
                m_archerMultipleShot[matchNum][i]->SetCritical(critical);
                m_archerMultipleShot[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_archerMultipleShot[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                count++;

                for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(i, SKILL_TYPE::ARCHER_MULTIPLE_SHOT, pos, vec3::Normalize(rotatedLook));
                }
                if (count >= 5)
                    break;
            }
        }

        return 0;
    }

    int CGameMgr::FireBall(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_figtherFireBall[matchNum][i]->active) {
                m_figtherFireBall[matchNum][i]->active = true;
                m_figtherFireBall[matchNum][i]->SetPos(pos);
                m_figtherFireBall[matchNum][i]->SetLook(look);
                m_figtherFireBall[matchNum][i]->SetPower(power);
                m_figtherFireBall[matchNum][i]->SetCritical(critical);
                m_figtherFireBall[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_figtherFireBall[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::RockThrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_ogreRockThrow[matchNum][i]->active) {
                m_ogreRockThrow[matchNum][i]->active = true;
                m_ogreRockThrow[matchNum][i]->SetPos(pos);
                m_ogreRockThrow[matchNum][i]->SetLook(look);
                m_ogreRockThrow[matchNum][i]->SetPower(power);
                m_ogreRockThrow[matchNum][i]->SetCritical(critical);
                m_ogreRockThrow[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_ogreRockThrow[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::DimensionCrush(int matchNum, const vec3& pos, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_ogreDimensionCrush[matchNum][i]->active) {
                m_ogreDimensionCrush[matchNum][i]->active = true;
                m_ogreDimensionCrush[matchNum][i]->SetPos(pos);
                m_ogreDimensionCrush[matchNum][i]->SetPower(power);
                m_ogreDimensionCrush[matchNum][i]->SetCritical(critical);
                m_ogreDimensionCrush[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_ogreDimensionCrush[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::DimensionCrush(int matchNum, int id)
    {
        if (m_ogreDimensionCrush[matchNum][id]->active) {
            m_ogreDimensionCrush[matchNum][id]->active = false;
            for (int clientID : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
                if (clientID == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(clientID)->GetPacketSender()->SendRemoveSkillObjectPacket(id, SKILL_TYPE::OGRE_DIMENSION_CRUSH);
            }
        }
    }

    void CGameMgr::OgreCharging(int matchNum, int id)
    {
        m_ogreCharging[matchNum]->active = true;
        m_ogreCharging[matchNum]->SetClientID(id);
        m_ogreCharging[matchNum]->SetLook(CObjectMgr::GetInstance()->GetClient(id)->GetLook());
        m_skillMutex[matchNum].lock();
        m_activeSkills[matchNum].push_back(m_ogreCharging[matchNum]);
        m_skillMutex[matchNum].unlock();
    }

    int CGameMgr::ProAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proAttacks[matchNum][i]->active) {
                m_proAttacks[matchNum][i]->active = true;
                m_proAttacks[matchNum][i]->SetPos(pos);
                m_proAttacks[matchNum][i]->SetLook(look);
                m_proAttacks[matchNum][i]->SetPower(power);
                m_proAttacks[matchNum][i]->SetCritical(critical);
                m_proAttacks[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_proAttacks[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::ReturnZero(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proReturnZero[matchNum][i]->active) {
                m_proReturnZero[matchNum][i]->active = true;
                m_proReturnZero[matchNum][i]->SetPos(pos);
                m_proReturnZero[matchNum][i]->SetStartPos(pos);
                m_proReturnZero[matchNum][i]->SetLook(look);
                m_proReturnZero[matchNum][i]->SetPower(power);
                m_proReturnZero[matchNum][i]->SetCritical(critical);
                m_proReturnZero[matchNum][i]->SetClientID(clientID);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_proReturnZero[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::HelloWorld(int matchNum, const vec3& pos, int clientID, const std::vector<int>& ids)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proHelloWorld[matchNum][i]->active) {
                m_proHelloWorld[matchNum][i]->active = true;
                m_proHelloWorld[matchNum][i]->SetPos(pos);
                m_proHelloWorld[matchNum][i]->SetClientID(clientID);
                m_proHelloWorld[matchNum][i]->SetArea(ids);
                m_skillMutex[matchNum].lock();
                m_activeSkills[matchNum].push_back(m_proHelloWorld[matchNum][i]);
                m_skillMutex[matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::HelloWorld(int matchNum, int objectID)
    {
        if (m_proHelloWorld[matchNum][objectID]->active) {
            m_proHelloWorld[matchNum][objectID]->active = false;
            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)) {
                if (id == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(objectID, SKILL_TYPE::PRO_HELLO_WORLD);
            }
        }
    }
}