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
		void InitializeBoundingBox(const vec3& _boxCenter, const vec3& _boxExtent, float _scaleValue);

		void SetInitializeStat(int _hp, int _mp, int _attack, int _magic, int _defense, int _regist, int _speed, int _tentacity, int _critical);

		//Getter, Setter
		CPacketSender* GetPacketSender() const { return m_packetSender; }
		void SetState(const CL_STATE& _state) { m_state = _state; }
		const CL_STATE& GetState() const { return m_state; }
		bool IsDisconnected() const { return m_isDisconnected; }

		void SetUpdateTime() { m_updateTime = TimeUtil::CurTime(); }
		float GetElapsedTime() const {return std::chrono::duration_cast<std::chrono::microseconds>(TimeUtil::CurTime() - m_updateTime).count() * 0.000001f;}

		void SetMatchId(int _id) { m_matchID = _id; }
		int GetMatchId() const { return m_matchID; }

		void SetName(std::string _name) { m_name = _name; }
		const std::string& GetName() const { return m_name; }

		void SetUsingSkill(bool _use) { m_usingSkill = _use; }
		void SetPlayerJob(int _job) { m_job = _job; }
		void SetUsingSkillType(SKILL_TYPE _type) { m_usingSkillType = _type; }
		bool GetUsingSkill() const { return m_usingSkill; }
		int GetPlayerJob() const { return m_job; }
		const SKILL_TYPE GetUsingSkillType() const { return m_usingSkillType; }

		const std::chrono::system_clock::time_point& GetSkillLastUsedTime(int _index) const { return m_skills[_index]->GetLastSkillTime(); }
		const int GetSkillNum(int _index) { return m_skills[_index]->GetSkillNum(); }
		int GetSkillCoolTime(int _index) { return m_skills[_index]->GetSkillCoolTime(); }
		int GetMpConsumption(int _index) const { return m_skills[_index]->GetMpConsumption(); }
		void SetSkillNum(int _index, int _skillNum) { m_skills[_index]->SetSkillNum(_skillNum); }
		void SetSkillCoolTime(int _index, int _coolTime) { m_skills[_index]->SetCoolTime(_coolTime); }
		void SetSkillLastUsedTime(int _index) { m_skills[_index]->SetLastSkillTime(TimeUtil::CurTime()); }
		void SetMpConsumption(int _index, int _mp) { m_skills[_index]->SetMpConsumption(_mp); }

		void SetModelCustomize(const ModelCustomize& _model) { m_model = _model; }
		const ModelCustomize& GetModelCustomize() const { return m_model; }

		void SetJump(bool _jump) { m_jump = _jump; }
		void SetJumpNum(int _num) { m_jumpNum = _num; }
		bool GetJump() const { return m_jump; }

		bool IsDead() const { return m_isDead; }

		void SetTeleport(bool _teleport) { m_teleport = _teleport; if (_teleport) m_isTeleportCool = true; }
		void SetTeleportNum(int _num) { m_teleportNum = _num; }
		bool GetTeleport() const { return m_teleport; }
		void SetTeleportLastUsedTime() { m_teleportLastUsedTime = TimeUtil::CurTime(); }
		const std::chrono::system_clock::time_point& GetTeleportLastUsedTime() const { return m_teleportLastUsedTime; }
		bool CheckTeleportCoolTime();

		int GetAccumulatedDamage() const { return m_accumulatedDamage; }
		void ResetAccumulatedDamage() { m_accumulatedDamage = 0; }

		void SetGold(short _gold) { m_gold = _gold; }
		short GetGold() const { return m_gold; }

		CItemInfo* GetItemInfo(int _index) const { return m_itemInfos[_index]; }

		CStatus* GetStatus() const { return m_status.get(); }

		virtual void Move(float _elapsedTime);
		void Jump(float _elapsedTime);
		void Teleport(float _elapsedTime);
		void Damage(int _power, int _critical, DAMAGE_TYPE _type, int _objectID);

		void RecvProcess(const DWORD& _bytes, OverlapEx* _over_ex);
		void Disconnect();

		std::shared_mutex m_stateLock;
		std::mutex m_statLock;

		bool Update(float _elapsedTime) override;

	private:
		//Defensive Functions
		bool NoDefensive(int _damage, int _id);
		bool Reflect(int _damage, int _id);
		bool DefensiveStance(int _damage, int _id);
		bool Indestructible(int _damage, int _id);
		bool Counter(int _damage, int _id);
		bool Endure(int _damage, int _id);

		void ProcessDeath(int _killerID);
		void ProcessRespawn(TimePoint _now);
		void ProcessBaseHeal(TimePoint _now);
		void ProcessTeleportCoolTime(TimePoint _now);

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