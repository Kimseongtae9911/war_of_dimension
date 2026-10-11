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
		float m_readyTime = 120.f;
		float m_gameTime = 0.f;
		std::chrono::system_clock::time_point m_lastTime;

		bool m_nexusAttackPossible = false;

		//Teleport, Magnetic Fence
		bool m_teleport = false;
		bool m_fence = true;
		int m_gameOver = -1;

		void Reset() {
			m_readyTime = 120.0f;
			m_gameTime = 0.f;
			m_nexusAttackPossible = false;
			m_teleport = false;
			m_fence = true;
			m_gameOver = -1;
		}
	};

	struct ShopInfo
	{
		int m_level = 0;
		int m_currentPrice = 10;
	};

	class CGameMgr : public TSingleton<CGameMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;
		void Reset(int _match);

		bool UpdateReadyData(int _match, float _elapsedTime);
		float UpdateGameData(int _match);
		int GetHeroRespawnTime(int _match);

		void SkillAutoSelect(int _match);
		bool CheckCoolTime(std::shared_ptr<CClient> _client, char _type);

		float GetGameTime(int _match) { return m_gameData[_match]->m_gameTime; }
		static constexpr int m_fenceReleaseSeconds = 180;
		std::chrono::system_clock::time_point GetLastTime(int _match) { return  m_gameData[_match]->m_lastTime; }
		void SetLastTime(int _match) { m_gameData[_match]->m_lastTime = TimeUtil::CurTime(); }

		void SetTeleport(int _match, bool _tp) { m_gameData[_match]->m_teleport = _tp; }
		bool GetTeleport(int _match) const { return m_gameData[_match]->m_teleport; }
		void SetFence(int _match, bool _fence) { m_gameData[_match]->m_fence = _fence; }
		bool GetFence(int _match) const { return m_gameData[_match]->m_fence; }
		void SetNexusAttackPossible(int _match, bool _possible) { m_gameData[_match]->m_nexusAttackPossible = _possible; }
		bool GetNexusAttackPossible(int _match) { return m_gameData[_match]->m_nexusAttackPossible; }
		void SetPathNum(int _match, int _path) { m_pathNums[_match] = _path; }
		int GetPathNum(int _match) const { return m_pathNums[_match]; }

		int IsGameOver(int _match) const { return m_gameData[_match]->m_gameOver; }
		void GameOver(int _match, bool _heroWin) { m_gameData[_match]->m_gameOver = static_cast<int>(_heroWin); }

		void ActiveTower(bool _active, int _match, int _index);
		void TowerAttack(int _match, int _targetID, const vec3& _pos);
		CTower* GetTower(int _match, int _index) { return m_towers[_match][_index]; }
		CNexus* GetNexus(int _match) const { return m_nexus[_match]; }

		void BuyStat(int _match, int _index) { m_shopStatLevel[_match][_index].m_level++; }
		int GetStatLevel(int _match, int _index) { return m_shopStatLevel[_match][_index].m_level; }
		void SetCurrentPrice(int _match, int _index, int _price) { m_shopStatLevel[_match][_index].m_currentPrice = _price; }
		int GetCurrentPrice(int _match, int _index) const { return m_shopStatLevel[_match][_index].m_currentPrice; }

		//Wizard Skill
		int WizardAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int MagicMissle(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int MagicEye(int _matchNum, const vec3& _pos, const vec3& _look);
		void MagicEye(int _matchNum, int _id);
		int EnergyBall(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int BigBang(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);

		//Sworman Skill
		int AuraBlade(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int JudgeMentSword(int _matchNum, const vec3& _pos, int _power, int _critical, int _target, int _clientID);
		int ProtectedArea(int _matchNum, const vec3& _pos, const int _power, int _clientID);
		void ProtectedArea(int _matchNum, int _id);

		//Archer Skill
		int ArcherAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int StickyArrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int PhoenixArrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int PenetraitingShot(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int ArcherMultipleShot(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);

		//Fighter Skill
		int FireBall(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);

		//Ogre Skill
		int RockThrow(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int DimensionCrush(int _matchNum, const vec3& _pos, int _power, int _critical, int _clientID);
		void DimensionCrush(int _matchNum, int _id);
		void OgreCharging(int _matchNum, int _id);

		//Programmer Skill
		int ProAttack(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		int ReturnZero(int _matchNum, const vec3& _pos, const vec3& _look, int _power, int _critical, int _clientID);
		void WhileTrue(bool _active, int _matchNum) { m_proWhileTrue[_matchNum] = _active; }
		bool GetWhileTrue(int _matchNum) { return m_proWhileTrue[_matchNum]; }
		int HelloWorld(int _matchNum, const vec3& _pos, int _clientID, const std::vector<int>& _ids);
		void HelloWorld(int _matchNum, int _id);

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
