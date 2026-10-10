#pragma once

namespace wod_server {
	class CMonster : public CNpc
	{
	public:
		CMonster() {}
		~CMonster() {}

		void Initialize(uint8_t _posIndex) override;
		bool Update(float _elapsedTime) override;
		void Move(float _elapsedTime) override;
		void Chase(float _elapsedTime) override;
		virtual bool Damaged(int _clientID, int _power, DAMAGE_TYPE _type, bool _updateTarget = true) override;
		void Heal() override;
		void ReturnPos(float _elapsedTime);

		void SetInitPos(const vec3& _pos) { m_initPos = _pos; m_pos = _pos; }
		void SetInitLook(const vec3& _look) { m_initLook = _look; m_look = _look; }
		void SetRespawnNum(int _num) { m_respawnNum = _num; }
		int GetRespawnNum() const { return m_respawnNum; }
		void Respawn(int _currTime) override;

	protected:
		void SetMonsterInfo();
		void RegisterKillBuff(int _clientID);

	protected:
		int m_respawnTime = 0;
		int m_respawnNum = 0;
		int m_dieCount = 0;
		int m_gold = 0;
		float m_chaseDistance = 0.f;
		float m_attackDistance = 0.f;
		vec3 m_initPos = {};
		vec3 m_initLook = {};
		std::chrono::system_clock::time_point m_lastHealTime;
	};

	class CUniqueRed : public CMonster
	{
	public:
		CUniqueRed();
		~CUniqueRed() {}

		bool Update(float _elapsedTime) override;
		bool Damaged(int _clientID, int _power, DAMAGE_TYPE _type, bool _updateTarget = true) override;
	};

	class CRareGreen : public CMonster
	{
	public:
		CRareGreen();
		~CRareGreen() {}
	};

	class CRareGolem : public CMonster
	{
	public:
		CRareGolem();
		~CRareGolem() {}
	};

	class CNormalBear : public CMonster
	{
	public:
		CNormalBear();
		~CNormalBear() {}
	};

	class CNormalMinotaur : public CMonster
	{
	public:
		CNormalMinotaur();
		~CNormalMinotaur() {}
	};

	class CNormalChest : public CMonster
	{
	public:
		CNormalChest();
		~CNormalChest() {}
	};

	class CNormalBeholder : public CMonster
	{
	public:
		CNormalBeholder();
		~CNormalBeholder() {}
	};

}