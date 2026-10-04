#pragma once
#include "GameObject.h"
#include "CWizardAttack.h"
#include "CMagicMissile.h"
#include "CEnergyBall.h"
#include "CBigBang.h"
#include "CAuraBlade.h"
#include "CJudgementSword.h"
#include "CArcherAttack.h"
#include "CProtectedArea.h"
#include "CFireBall.h"
#include "CPhoenixArrow.h"
#include "CPenetraitingShot.h"
#include "CStickyArrow.h"
#include "CReturnZero.h"
#include "CProAttack.h"
#include "CHelloWorld.h"
#include "CRockThrow.h"
#include "CDimensionCrush.h"
#include "CCharging.h"
#include "CMultipleShot.h"
#include "CMagicEye.h"

namespace wod_server {
	class CTowerAttack;
	class CClient; 
	class CTower;
	class CNexus;

	struct GameData
	{
		float readyTime = 120.f;
		float gameTime = 0.f;
		std::chrono::system_clock::time_point lastTime;

		bool nexusAttackPossible = false;

		//Teleport, Magnetic Fence
		bool teleport = false;
		bool fence = true;
		int gameOver = -1;

		void Reset() {
			readyTime = 120.0f;
			gameTime = 0.f;
			nexusAttackPossible = false;
			teleport = false;
			fence = true;
			gameOver = -1;
		}
	};

	struct ShopInfo
	{
		int level = 0;
		int currentPrice = 10;
	};

	class CGameMgr : public TSingleton<CGameMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;
		void Reset(int match);

		bool UpdateReadyData(int match, float elapsedTime);
		float UpdateGameData(int match);
		int GetHeroRespawnTime(int match);

		void SkillAutoSelect(int match);
		bool CheckCoolTime(std::shared_ptr<CClient> client, char type);

		float GetGameTime(int match) { return m_gameData[match]->gameTime; }
		std::chrono::system_clock::time_point GetLastTime(int match) { return  m_gameData[match]->lastTime; }
		void SetLastTime(int match) { m_gameData[match]->lastTime = TimeUtil::CurTime(); }

		void SetTeleport(int match, bool tp) { m_gameData[match]->teleport = tp; }
		bool GetTeleport(int match) const { return m_gameData[match]->teleport; }
		void SetFence(int match, bool fence) { m_gameData[match]->fence = fence; }
		bool GetFence(int match) const { return m_gameData[match]->fence; }
		void SetNexusAttackPossible(int match, bool possible) { m_gameData[match]->nexusAttackPossible = possible; }
		bool GetNexusAttackPossible(int match) { return m_gameData[match]->nexusAttackPossible; }
		void SetPathNum(int match, int path) { m_pathNums[match] = path; }
		int GetPathNum(int match) const { return m_pathNums[match]; }

		int IsGameOver(int match) const { return m_gameData[match]->gameOver; }
		void GameOver(int match, bool heroWin) { m_gameData[match]->gameOver = static_cast<int>(heroWin); }

		void ActiveTower(bool active, int match, int index);
		void TowerAttack(int match, int targetID, const vec3& pos);
		CTower* GetTower(int match, int index) { return m_towers[match][index]; }
		CNexus* GetNexus(int match) const { return m_nexus[match]; }

		void BuyStat(int match, int index) { m_shopStatLevel[match][index].level++; }
		int GetStatLevel(int match, int index) { return m_shopStatLevel[match][index].level; }
		void SetCurrentPrice(int match, int index, int price) { m_shopStatLevel[match][index].currentPrice = price; }
		int GetCurrentPrice(int match, int index) const { return m_shopStatLevel[match][index].currentPrice; }

		//Wizard Skill
		int WizardAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int MagicMissle(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int MagicEye(int matchNum, const vec3& pos, const vec3& look);
		void MagicEye(int matchNum, int id);
		int EnergyBall(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int BigBang(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);

		//Sworman Skill
		int AuraBlade(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int JudgeMentSword(int matchNum, const vec3& pos, int power, int critical, int target, int clientID);
		int ProtectedArea(int matchNum, const vec3& pos, const int power, int clientID);
		void ProtectedArea(int matchNum, int id);

		//Archer Skill
		int ArcherAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int StickyArrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int PhoenixArrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int PenetraitingShot(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int ArcherMultipleShot(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);

		//Fighter Skill
		int FireBall(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);

		//Ogre Skill
		int RockThrow(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int DimensionCrush(int matchNum, const vec3& pos, int power, int critical, int clientID);
		void DimensionCrush(int matchNum, int id);
		void OgreCharging(int matchNum, int id);

		//Programmer Skill
		int ProAttack(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		int ReturnZero(int matchNum, const vec3& pos, const vec3& look, int power, int critical, int clientID);
		void WhileTrue(bool active, int matchNum) { m_proWhileTrue[matchNum] = active; }
		bool GetWhileTrue(int matchNum) { return m_proWhileTrue[matchNum]; }
		int HelloWorld(int matchNum, const vec3& pos, int clientID, const std::vector<int>& ids);
		void HelloWorld(int matchNum, int id);

	private:
		//Time, Teleport, Fence
		std::array<GameData*, MAX_MATCH> m_gameData;

		//Path
		std::array<int, MAX_MATCH> m_pathNums;
		
		//Tower
		std::array<std::array<CTower*, PATH_NUM>, MAX_MATCH> m_towers;
		std::array<std::array<CTowerAttack*, PATH_NUM>, MAX_MATCH> m_towerAttack;
		std::array<CNexus*, MAX_MATCH> m_nexus;

		//Skill
		std::array<std::vector<CGameObject*>, MAX_MATCH> m_activeSkills;
		std::array<std::mutex, MAX_MATCH> m_skillMutex;
		std::array<std::array<CWizardAttack*, MAX_SKILL_OBJECT>, MAX_MATCH> m_wizardAttacks;
		std::array<std::array<CMagicMissile*, MAX_SKILL_OBJECT>, MAX_MATCH> m_wizardMissiles;
		std::array<std::array<CMagicEye*, MAX_SKILL_OBJECT>, MAX_MATCH> m_wizardMagicEyes;
		std::array<std::array<CEnergyBall*, MAX_SKILL_OBJECT>, MAX_MATCH> m_wizardEnergyBalls;
		std::array<std::array<CBigBang*, MAX_SKILL_OBJECT>, MAX_MATCH> m_wizardBigBang;

		std::array<std::array<CAuraBlade*, MAX_SKILL_OBJECT>, MAX_MATCH> m_swordManAuraBlade;
		std::array<std::array<CJudgementSword*, MAX_SKILL_OBJECT>, MAX_MATCH> m_swordManJudgementSword;
		std::array<std::array<CProtectedArea*, MAX_SKILL_OBJECT>, MAX_MATCH> m_swordManProtectedArea;

		std::array<std::array<CArcherAttack*, MAX_SKILL_OBJECT>, MAX_MATCH> m_archerAttacks;
		std::array<std::array<CStickyArrow*, MAX_SKILL_OBJECT>, MAX_MATCH> m_archerStickyArrows;
		std::array<std::array<CPhoenixArrow*, MAX_SKILL_OBJECT>, MAX_MATCH> m_archerPhoenixArrows;
		std::array<std::array<CPenetraitingShot*, MAX_SKILL_OBJECT>, MAX_MATCH> m_archerPenetraitingShot;
		std::array<std::array<CMultipleShot*, MAX_SKILL_OBJECT * 5>, MAX_MATCH> m_archerMultipleShot;

		std::array<std::array<CFireBall*, MAX_SKILL_OBJECT>, MAX_MATCH> m_figtherFireBall;

		std::array<std::array<CRockThrow*, MAX_SKILL_OBJECT>, MAX_MATCH> m_ogreRockThrow;
		std::array<std::array<CDimensionCrush*, MAX_SKILL_OBJECT>, MAX_MATCH> m_ogreDimensionCrush; 
		std::array<CCharging*, MAX_MATCH> m_ogreCharging;

		std::array<std::array<CProAttack*, MAX_SKILL_OBJECT>, MAX_MATCH> m_proAttacks;
		std::array<std::array<CReturnZero*, MAX_SKILL_OBJECT>, MAX_MATCH> m_proReturnZero;
		std::array<std::array<CHelloWorld*, MAX_SKILL_OBJECT>, MAX_MATCH> m_proHelloWorld;
		std::array<bool, MAX_MATCH> m_proWhileTrue;

		//Shop
		std::array<std::array<ShopInfo, 9>, MAX_MATCH> m_shopStatLevel;
	};
}