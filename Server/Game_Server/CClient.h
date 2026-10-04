#pragma once
#include "CSkill.h"
#include "CItem.h"
#include "CPacketSender.h"
#include "CStatus.h"

namespace wod_server {

	class JobQueue;
	class CClient : public CMoveObject, public std::enable_shared_from_this<CClient>
	{
	public:
		CClient();
		virtual ~CClient();

		void Initialize();
		void InitializeBoundingBox(const vec3& boxCenter, const vec3& boxExtent, float scaleValue);

		void SetInitializeStat(int hp, int mp, int attack, int magic, int defense, int regist, int speed, int tentacity, int critical);

		//Getter, Setter
		CPacketSender* GetPacketSender() const { return m_packetSender; }
		void SetState(const CL_STATE& state) { m_state = state; }
		const CL_STATE& GetState() const { return m_state; }
		bool IsDisconnected() const { return m_isDisconnected; }

		void SetUpdateTime() { m_updateTime = TimeUtil::CurTime(); }
		float GetElapsedTime() const {return std::chrono::duration_cast<std::chrono::microseconds>(TimeUtil::CurTime() - m_updateTime).count() * 0.000001f;}

		void SetMatchId(int id) { m_matchID = id; }
		int GetMatchId() const { return m_matchID; }

		void SetName(std::string name) { m_name = name; }
		const std::string& GetName() const { return m_name; }

		void SetUsingSkill(bool use) { m_usingSkill = use; }
		void SetPlayerJob(int job) { m_job = job; }
		void SetUsingSkillType(SKILL_TYPE type) { m_usingSkillType = type; }
		bool GetUsingSkill() const { return m_usingSkill; }
		int GetPlayerJob() const { return m_job; }
		const SKILL_TYPE GetUsingSkillType() const { return m_usingSkillType; }		
		
		const std::chrono::system_clock::time_point& GetSkillLastUsedTime(int index) const { return m_skills[index]->GetLastSkillTime(); }
		const int GetSkillNum(int index) { return m_skills[index]->GetSkillNum(); }
		int GetSkillCoolTime(int index) { return m_skills[index]->GetSkillCoolTime(); }
		int GetMpConsumption(int index) const { return m_skills[index]->GetMpConsumption(); }
		void SetSkillNum(int index, int skillNum) { m_skills[index]->SetSkillNum(skillNum); }
		void SetSkillCoolTime(int index, int coolTime) { m_skills[index]->SetCoolTime(coolTime); }
		void SetSkillLastUsedTime(int index) { m_skills[index]->SetLastSkillTime(TimeUtil::CurTime()); }
		void SetMpConsumption(int index, int mp) { m_skills[index]->SetMpConsumption(mp); }

		void SetModelCustomize(const ModelCustomize& model) { m_model = model; }
		const ModelCustomize& GetModelCustomize() const { return m_model; }

		void SetJump(bool jump) { m_jump = jump; }
		void SetJumpNum(int num) { m_jumpNum = num; }
		bool GetJump() const { return m_jump; }

		bool IsDead() const { return m_isDead; }

		void SetTeleport(bool teleport) { m_teleport = teleport; if (teleport) m_isTeleportCool = true; }
		void SetTeleportNum(int num) { m_teleportNum = num; }
		bool GetTeleport() const { return m_teleport; }
		void SetTeleportLastUsedTime() { m_teleportLastUsedTime = TimeUtil::CurTime(); }
		const std::chrono::system_clock::time_point& GetTeleportLastUsedTime() const { return m_teleportLastUsedTime; }
		bool CheckTeleportCoolTime();

		int GetAccumulatedDamage() const { return m_accumulatedDamage; }
		void ResetAccumulatedDamage() { m_accumulatedDamage = 0; }

		void SetGold(short gold) { m_gold = gold; }
		short GetGold() const { return m_gold; }

		CItemInfo* GetItemInfo(int index) const { return m_itemInfos[index]; }

		CStatus* GetStatus() const { return m_status.get(); }

		virtual void Move(float elapsedTime);
		void Jump(float elapsedTime);
		void Teleport(float elapsedTime);
		void Damage(int power, int critical, DAMAGE_TYPE type, int objectID);

		void RecvProcess(const DWORD& bytes, OverlapEx* over_ex);
		void Disconnect();

		std::shared_mutex stateLock;
		std::mutex statLock;		

		bool Update(float elapsedTime) override;

	private:
		//Defensive Functions
		bool NoDefensive(int damage, int id);
		bool Reflect(int damage, int id);
		bool DefensiveStance(int damage, int id);
		bool Indestructible(int damage, int id);
		bool Counter(int damage, int id);
		bool Endure(int damage, int id);

		void ProcessDeath(int killerID);
		void ProcessRespawn(TimePoint now);
		void ProcessBaseHeal(TimePoint now);
		void ProcessTeleportCoolTime(TimePoint now);

	private:
		CL_STATE m_state = CL_STATE::ST_FREE;
		CPacketSender* m_packetSender;

		std::chrono::system_clock::time_point m_updateTime = {};

		//Skill Container
		int m_job = 0;
		std::array<std::shared_ptr<CSkill>, MAX_SKILL + 1> m_skills;// basic attack + skills
		SKILL_TYPE m_usingSkillType = SKILL_TYPE::NONE;
		bool m_usingSkill = false;
		int m_skillNum = 0;
		
		//Item
		std::array<CItemInfo*, ITEM_NUM> m_itemInfos;

		//Teleport
		std::chrono::system_clock::time_point m_teleportLastUsedTime;
		bool m_isTeleportCool = false;

		ModelCustomize m_model;
		std::string m_name;

		int m_matchID = -1;

		bool m_jump = false;
		int m_jumpNum = 0;
		float m_jumpTime = 0.f;

		bool m_teleport = false;
		int m_teleportNum = 0;

		std::shared_ptr<CStatus> m_status;

		short m_gold = 0;

		//For Counter, Endure, Gluttony
		int m_accumulatedDamage = 0;
		
		bool m_isDisconnected = false;

		std::unordered_map<DEFENSIVE_BUFF, std::function<bool(int, int)>> m_defensiveFunction;

		std::atomic_bool m_isEnqueued = false;

		// Respawn
		bool m_isDead = false;
		TimePoint m_deathTime = {};
		
		// Heal
		TimePoint m_lastBaseHeal = {};
	};

}