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

    void CGameMgr::Reset(int _match)
    {
        m_gameData[_match]->Reset();

        for (int i = 0; i < m_shopStatLevel[0].size(); ++i) {
            m_shopStatLevel[_match][i].m_currentPrice = 10;
            m_shopStatLevel[_match][i].m_level = 0;
        }

        for (int i = 0; i < PATH_NUM; ++i) {
            m_towers[_match][i]->Reset();
            m_towerAttack[_match][i]->m_active = false;
        }
        m_nexus[_match]->Reset();

        //Skill
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            m_wizardAttacks[_match][i]->m_active = false;
            m_wizardMissiles[_match][i]->m_active = false;
            m_wizardMagicEyes[_match][i]->m_active = false;
            m_wizardEnergyBalls[_match][i]->m_active = false;
            m_wizardBigBang[_match][i]->m_active = false;
            m_swordManAuraBlade[_match][i]->m_active = false;
            m_swordManJudgementSword[_match][i]->m_active = false;
            m_swordManProtectedArea[_match][i]->m_active = false;
            m_archerAttacks[_match][i]->m_active = false;
            m_archerStickyArrows[_match][i]->m_active = false;
            m_archerPhoenixArrows[_match][i]->m_active = false;
            m_archerPenetraitingShot[_match][i]->m_active = false;
            m_figtherFireBall[_match][i]->m_active = false;
            m_ogreRockThrow[_match][i]->m_active = false;
            m_ogreDimensionCrush[_match][i]->m_active = false;
            m_proReturnZero[_match][i]->m_active = false;
            m_proHelloWorld[_match][i]->m_active = false;
            m_proAttacks[_match][i]->m_active = false;
        }
        for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
            m_archerMultipleShot[_match][i]->m_active = false;
        }
        m_ogreCharging[_match]->m_active = false;
        m_proWhileTrue[_match] = false;
        m_skillMutex[_match].lock();
        m_activeSkills[_match].clear();
        m_skillMutex[_match].unlock();
    }

    float CGameMgr::UpdateGameData(int _match)
    {
        //Time Update
        float elapsedTime = TimeUtil::CalElapsedTime(m_gameData[_match]->m_lastTime);
        m_gameData[_match]->m_lastTime = TimeUtil::CurTime();
        m_gameData[_match]->m_gameTime += elapsedTime;

        //Tower Update
        for (int i = 0; i < m_towers[_match].size(); ++i) {
            m_towers[_match][i]->Update(_match);
        }
        for (int i = 0; i < m_towerAttack[_match].size(); ++i) {
            m_towerAttack[_match][i]->Update(elapsedTime);
        }

        std::vector<CGameObject*> activeSkills;
        std::vector<CGameObject*> updatedSkills;
        m_skillMutex[_match].lock();
        activeSkills = m_activeSkills[_match];
        m_skillMutex[_match].unlock();
        for (CGameObject* skill : activeSkills)
        {
            if (skill->Update(elapsedTime))
                updatedSkills.push_back(skill);
        }

        {
            std::lock_guard<std::mutex> lock(m_skillMutex[_match]);
            m_activeSkills[_match] = std::move(updatedSkills);
        }

        //Send Time
        for (int i = 0; i < MAX_PLAYER; ++i) {
            if (CMatchMgr::GetInstance()->GetMatchPlayers(_match)[i] == -1)
                continue;
            CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[i])->GetPacketSender()->SendGameTimePacket(static_cast<int>(::ceil(m_gameData[_match]->m_gameTime)), 1);
        }

        //Check Fence Activation
        if (m_gameData[_match]->m_fence && IsFloatEqual(::floor(m_gameData[_match]->m_gameTime), static_cast<float>(m_fenceReleaseSeconds))) {
            m_gameData[_match]->m_fence = false;
        }

        return elapsedTime;
    }

    int CGameMgr::GetHeroRespawnTime(int _match)
    {
        int respawnTime = ClientInfos::HERO_INIT_RESPAWN_TIME; // add time from calculation by game time

        return respawnTime;
    }

    void CGameMgr::SkillAutoSelect(int _match)
    {
        int playerSkill = PLAYER_SKILL / 4;
        int bossSkill = BOSS_SKILL / 2;
        for (int i = 0; i < MAX_PLAYER; ++i) {
            std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[i]);

            const int bossJob = client->GetPlayerJob() >= MAX_JOB ? client->GetPlayerJob() - MAX_JOB : client->GetPlayerJob();
            for (int j = 1; j < MAX_SKILL + 1; ++j) {
                if (client->GetSkillNum(j) == 0) {
                    if (j == MAX_SKILL) { //Ultimate
                        if (i == 3) {
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM);

                            client->SetSkillNum(j, bossSkill + bossJob * BOSS_SKILL_NUM + BOSS_SKILL_NUM - 1 - (21 - 96 - 1));
                        }
                        else {
                            for (int k = 0; k < MAX_PLAYER; ++k) {
                                if (i == k)
                                    continue;
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, playerSkill + client->GetPlayerJob() * PLAYER_SKILL_NUM + PLAYER_SKILL_NUM - 1);
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
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill + 1);
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
                                CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(_match)[k])->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill);
                            }
                            client->GetPacketSender()->SendSelectSkillPacket(i, j - 1, skill);
                           client->SetSkillNum(j, skill);
                        }
                    }
                }
            }
        }
    }

    bool CGameMgr::CheckCoolTime(std::shared_ptr<CClient> _client, char _type)
    {
        if (_client->GetUsingSkill()) {
            std::cout << "Using Skill" << std::endl;
            return false;
        }

        int skillCoolTime = _client->GetSkillCoolTime(static_cast<int>(_type) - 1);
        if (_client->GetStatus()->m_coolTimeBuff == COOLTIME_BUFF::OVERLOAD)
            skillCoolTime = static_cast<int>(skillCoolTime * 0.5f);
        if (!_client->GetUsingSkill() && std::chrono::duration_cast<std::chrono::seconds>(TimeUtil::CurTime() - _client->GetSkillLastUsedTime(static_cast<int>(_type) - 1)).count() >= skillCoolTime) {
            return true;
        }
        std::cout << "Skill CoolTime" << std::endl;
        return false;
    }

    void CGameMgr::ActiveTower(bool _active, int _match, int _index)
    {
        m_towers[_match][_index]->m_active = _active;
    }

    void CGameMgr::TowerAttack(int _match, int _targetID, const vec3& _pos)
    {
        for (int i = 0; i < m_towerAttack[_match].size(); ++i) {
            if (!m_towerAttack[_match][i]->m_active) {
                m_towerAttack[_match][i]->SetTarget(_targetID);
                m_towerAttack[_match][i]->SetPos(_pos);
                m_towerAttack[_match][i]->m_active = true;

                auto clients = CMatchMgr::GetInstance()->GetMatchPlayers(_match);
                for (int j = 0; j < MAX_PLAYER; ++j) {
                    if (-1 == clients[j])
                        continue;
                    CObjectMgr::GetInstance()->GetClient(clients[j])->GetPacketSender()->SendTowerAttackAddPacket(i, _pos);
                }

                break;
            }
        }
    }

    int CGameMgr::WizardAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardAttacks[_matchNum][i]->m_active) {
                m_wizardAttacks[_matchNum][i]->m_active = true;
                m_wizardAttacks[_matchNum][i]->SetPos(_pos);
                m_wizardAttacks[_matchNum][i]->SetLook(_look);
                m_wizardAttacks[_matchNum][i]->SetPower(_power);
                m_wizardAttacks[_matchNum][i]->SetCritical(_critical);
                m_wizardAttacks[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_wizardAttacks[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::MagicMissle(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardMissiles[_matchNum][i]->m_active) {
                m_wizardMissiles[_matchNum][i]->m_active = true;
                m_wizardMissiles[_matchNum][i]->SetPos(_pos);
                m_wizardMissiles[_matchNum][i]->SetLook(_look);
                m_wizardMissiles[_matchNum][i]->SetPower(_power);
                m_wizardMissiles[_matchNum][i]->SetCritical(_critical);
                m_wizardMissiles[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_wizardMissiles[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::MagicEye(int _matchNum, const vec3& _pos, const vec3& _look)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardMagicEyes[_matchNum][i]->m_active) {
                m_wizardMagicEyes[_matchNum][i]->m_active = true;
                m_wizardMagicEyes[_matchNum][i]->SetPos(_pos);
                m_wizardMagicEyes[_matchNum][i]->SetLook(_look);
                m_wizardMagicEyes[_matchNum][i]->ResetCheckTime();
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_wizardMagicEyes[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::MagicEye(int _matchNum, int _id)
    {
        if (m_wizardMagicEyes[_matchNum][_id]->m_active) {
            m_wizardMagicEyes[_matchNum][_id]->m_active = false;

            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
                if (-1 == id)
                    continue;

                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(id, SKILL_TYPE::WIZARD_MAGIC_EYE);
            }
        }
    }

    int CGameMgr::EnergyBall(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardEnergyBalls[_matchNum][i]->m_active) {
                m_wizardEnergyBalls[_matchNum][i]->m_active = true;
                m_wizardEnergyBalls[_matchNum][i]->SetPos(_pos);
                m_wizardEnergyBalls[_matchNum][i]->SetLook(_look);
                m_wizardEnergyBalls[_matchNum][i]->SetPower(_power);
                m_wizardEnergyBalls[_matchNum][i]->SetCritical(_critical);
                m_wizardEnergyBalls[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_wizardEnergyBalls[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::BigBang(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_wizardBigBang[_matchNum][i]->m_active) {
                m_wizardBigBang[_matchNum][i]->m_active = true;
                m_wizardBigBang[_matchNum][i]->SetPos(_pos);
                m_wizardBigBang[_matchNum][i]->SetLook(_look);
                m_wizardBigBang[_matchNum][i]->SetPower(_power);
                m_wizardBigBang[_matchNum][i]->SetCritical(_critical);
                m_wizardBigBang[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_wizardBigBang[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::AuraBlade(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManAuraBlade[_matchNum][i]->m_active) {
                m_swordManAuraBlade[_matchNum][i]->m_active = true;
                m_swordManAuraBlade[_matchNum][i]->SetPos(_pos);
                m_swordManAuraBlade[_matchNum][i]->SetLook(_look);
                m_swordManAuraBlade[_matchNum][i]->SetPower(_power);
                m_swordManAuraBlade[_matchNum][i]->SetCritical(_critical);
                m_swordManAuraBlade[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_swordManAuraBlade[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::JudgeMentSword(int _matchNum, const vec3& _pos, int _power, int _critical, int _target, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManJudgementSword[_matchNum][i]->m_active) {
                m_swordManJudgementSword[_matchNum][i]->m_active = true;
                m_swordManJudgementSword[_matchNum][i]->SetPos(_pos);
                m_swordManJudgementSword[_matchNum][i]->SetPower(_power);
                m_swordManJudgementSword[_matchNum][i]->SetTargetID(_target);
                m_swordManJudgementSword[_matchNum][i]->SetCritical(_critical);
                m_swordManJudgementSword[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_swordManJudgementSword[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::ProtectedArea(int _matchNum, const vec3& _pos, const int _power, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_swordManProtectedArea[_matchNum][i]->m_active) {
                m_swordManProtectedArea[_matchNum][i]->m_active = true;
                m_swordManProtectedArea[_matchNum][i]->SetPos(_pos);
                m_swordManProtectedArea[_matchNum][i]->SetDefensePower(_power);
                m_swordManProtectedArea[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_swordManProtectedArea[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::ProtectedArea(int _matchNum, int _objectID)
    {
        if (m_swordManProtectedArea[_matchNum][_objectID]->m_active) {
            m_swordManProtectedArea[_matchNum][_objectID]->m_active = false;
            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
                if (id == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(_objectID, SKILL_TYPE::SWORDMAN_PROTECTED_AREA);
            }
        }
    }

    int CGameMgr::ArcherAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerAttacks[_matchNum][i]->m_active) {
                m_archerAttacks[_matchNum][i]->m_active = true;
                m_archerAttacks[_matchNum][i]->SetPos(_pos);
                m_archerAttacks[_matchNum][i]->SetLook(_look);
                m_archerAttacks[_matchNum][i]->SetPower(_power);
                m_archerAttacks[_matchNum][i]->SetCritical(_critical);
                m_archerAttacks[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_archerAttacks[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::StickyArrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerStickyArrows[_matchNum][i]->m_active) {
                m_archerStickyArrows[_matchNum][i]->m_active = true;
                m_archerStickyArrows[_matchNum][i]->SetPos(_pos);
                m_archerStickyArrows[_matchNum][i]->SetLook(_look);
                m_archerStickyArrows[_matchNum][i]->SetPower(_power);
                m_archerStickyArrows[_matchNum][i]->SetCritical(_critical);
                m_archerStickyArrows[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_archerStickyArrows[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::PhoenixArrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerPhoenixArrows[_matchNum][i]->m_active) {
                m_archerPhoenixArrows[_matchNum][i]->m_active = true;
                m_archerPhoenixArrows[_matchNum][i]->SetPos(_pos);
                m_archerPhoenixArrows[_matchNum][i]->SetLook(_look);
                m_archerPhoenixArrows[_matchNum][i]->SetPower(_power);
                m_archerPhoenixArrows[_matchNum][i]->SetCritical(_critical);
                m_archerPhoenixArrows[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_archerPhoenixArrows[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::PenetraitingShot(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_archerPenetraitingShot[_matchNum][i]->m_active) {
                m_archerPenetraitingShot[_matchNum][i]->m_active = true;
                m_archerPenetraitingShot[_matchNum][i]->SetPos(_pos);
                m_archerPenetraitingShot[_matchNum][i]->SetLook(_look);
                m_archerPenetraitingShot[_matchNum][i]->SetPower(_power);
                m_archerPenetraitingShot[_matchNum][i]->SetCritical(_critical);
                m_archerPenetraitingShot[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_archerPenetraitingShot[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::ArcherMultipleShot(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        float angleIncrement = 20.0f;
        int numArrows = 5;

        float startAngle = -angleIncrement * (numArrows / 2);
        int count = 0;

        for (int i = 0; i < MAX_SKILL_OBJECT * 5; ++i) {
            float angle = startAngle + angleIncrement * count;
            float radians = angle * (DirectX::XM_PI / 180.0f);
            vec3 rotatedLook = vec3(_look.m_x * cos(radians) + _look.m_z * sin(radians), _look.m_y, _look.m_z * cos(radians) - _look.m_x * sin(radians));

            if (!m_archerMultipleShot[_matchNum][i]->m_active) {
                m_archerMultipleShot[_matchNum][i]->m_active = true;
                m_archerMultipleShot[_matchNum][i]->SetPos(_pos);
                m_archerMultipleShot[_matchNum][i]->SetLook(vec3::Normalize(rotatedLook));
                m_archerMultipleShot[_matchNum][i]->SetPower(_power);
                m_archerMultipleShot[_matchNum][i]->SetCritical(_critical);
                m_archerMultipleShot[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_archerMultipleShot[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                count++;

                for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
                    if (-1 == id)
                        continue;
                    CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(i, SKILL_TYPE::ARCHER_MULTIPLE_SHOT, _pos, vec3::Normalize(rotatedLook));
                }
                if (count >= 5)
                    break;
            }
        }

        return 0;
    }

    int CGameMgr::FireBall(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_figtherFireBall[_matchNum][i]->m_active) {
                m_figtherFireBall[_matchNum][i]->m_active = true;
                m_figtherFireBall[_matchNum][i]->SetPos(_pos);
                m_figtherFireBall[_matchNum][i]->SetLook(_look);
                m_figtherFireBall[_matchNum][i]->SetPower(_power);
                m_figtherFireBall[_matchNum][i]->SetCritical(_critical);
                m_figtherFireBall[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_figtherFireBall[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::RockThrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_ogreRockThrow[_matchNum][i]->m_active) {
                m_ogreRockThrow[_matchNum][i]->m_active = true;
                m_ogreRockThrow[_matchNum][i]->SetPos(_pos);
                m_ogreRockThrow[_matchNum][i]->SetLook(_look);
                m_ogreRockThrow[_matchNum][i]->SetPower(_power);
                m_ogreRockThrow[_matchNum][i]->SetCritical(_critical);
                m_ogreRockThrow[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_ogreRockThrow[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::DimensionCrush(int _matchNum, const vec3& _pos, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_ogreDimensionCrush[_matchNum][i]->m_active) {
                m_ogreDimensionCrush[_matchNum][i]->m_active = true;
                m_ogreDimensionCrush[_matchNum][i]->SetPos(_pos);
                m_ogreDimensionCrush[_matchNum][i]->SetPower(_power);
                m_ogreDimensionCrush[_matchNum][i]->SetCritical(_critical);
                m_ogreDimensionCrush[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_ogreDimensionCrush[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::DimensionCrush(int _matchNum, int _id)
    {
        if (m_ogreDimensionCrush[_matchNum][_id]->m_active) {
            m_ogreDimensionCrush[_matchNum][_id]->m_active = false;
            for (int clientID : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
                if (clientID == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(clientID)->GetPacketSender()->SendRemoveSkillObjectPacket(_id, SKILL_TYPE::OGRE_DIMENSION_CRUSH);
            }
        }
    }

    void CGameMgr::OgreCharging(int _matchNum, int _id)
    {
        m_ogreCharging[_matchNum]->m_active = true;
        m_ogreCharging[_matchNum]->SetClientID(_id);
        m_ogreCharging[_matchNum]->SetLook(CObjectMgr::GetInstance()->GetClient(_id)->GetLook());
        m_skillMutex[_matchNum].lock();
        m_activeSkills[_matchNum].push_back(m_ogreCharging[_matchNum]);
        m_skillMutex[_matchNum].unlock();
    }

    int CGameMgr::ProAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proAttacks[_matchNum][i]->m_active) {
                m_proAttacks[_matchNum][i]->m_active = true;
                m_proAttacks[_matchNum][i]->SetPos(_pos);
                m_proAttacks[_matchNum][i]->SetLook(_look);
                m_proAttacks[_matchNum][i]->SetPower(_power);
                m_proAttacks[_matchNum][i]->SetCritical(_critical);
                m_proAttacks[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_proAttacks[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    int CGameMgr::ReturnZero(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proReturnZero[_matchNum][i]->m_active) {
                m_proReturnZero[_matchNum][i]->m_active = true;
                m_proReturnZero[_matchNum][i]->SetPos(_pos);
                m_proReturnZero[_matchNum][i]->SetStartPos(_pos);
                m_proReturnZero[_matchNum][i]->SetLook(_look);
                m_proReturnZero[_matchNum][i]->SetPower(_power);
                m_proReturnZero[_matchNum][i]->SetCritical(_critical);
                m_proReturnZero[_matchNum][i]->SetClientID(_clientID);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_proReturnZero[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }

        return 0;
    }

    int CGameMgr::HelloWorld(int _matchNum, const vec3& _pos, int _clientID, const std::vector<int>& _ids)
    {
        for (int i = 0; i < MAX_SKILL_OBJECT; ++i) {
            if (!m_proHelloWorld[_matchNum][i]->m_active) {
                m_proHelloWorld[_matchNum][i]->m_active = true;
                m_proHelloWorld[_matchNum][i]->SetPos(_pos);
                m_proHelloWorld[_matchNum][i]->SetClientID(_clientID);
                m_proHelloWorld[_matchNum][i]->SetArea(_ids);
                m_skillMutex[_matchNum].lock();
                m_activeSkills[_matchNum].push_back(m_proHelloWorld[_matchNum][i]);
                m_skillMutex[_matchNum].unlock();
                return i;
            }
        }
        return 0;
    }

    void CGameMgr::HelloWorld(int _matchNum, int _objectID)
    {
        if (m_proHelloWorld[_matchNum][_objectID]->m_active) {
            m_proHelloWorld[_matchNum][_objectID]->m_active = false;
            for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_matchNum)) {
                if (id == -1)
                    continue;
                CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(_objectID, SKILL_TYPE::PRO_HELLO_WORLD);
            }
        }
    }
}
