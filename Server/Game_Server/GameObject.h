#pragma once

namespace wod_server {
	class Session;

	class CGameObject
	{
	public:
		CGameObject() {};
		virtual ~CGameObject() {};

		const vec3& GetPos() const { return m_pos; }
		void SetPos(const float _x, const float _y, const float _z) { m_pos.m_x = _x, m_pos.m_y = _y, m_pos.m_z = _z; }
		void SetPos(const vec3& _pos) { m_pos = _pos; }
		void SetPos(const vec2& _pos) { m_pos.m_x = _pos.m_x; m_pos.m_z = _pos.m_z; }

		const int GetID() const { return m_id; }
		void SetID(const int _id) { m_id = _id; }

		virtual bool Update(float _elapsedTime) { return true; }

		const DirectX::BoundingOrientedBox& GetBoundingBox() const { return m_boundingBox; }
		const DirectX::XMFLOAT4X4& GetWorldMatrix() const { return m_worldMatrix; }

	protected:
		vec3 m_pos;
		int m_id;

		DirectX::BoundingOrientedBox m_boundingBox;
		DirectX::BoundingOrientedBox m_initBoundingBox;
		DirectX::XMFLOAT4X4 m_worldMatrix;
	};


	class CMoveObject : public CGameObject
	{
	public:
		CMoveObject() {};
		virtual ~CMoveObject() {};

		char GetDir() const { return m_dir; }
		void SetDir(const char _dir) { m_dir = _dir; }

		const vec3& GetLook() const { return m_look; }
		const vec3& GetUp() const { return m_up; }
		const vec3& GetRight() const { return m_right; }
		void SetLook(const vec3& _look) { m_look = _look; m_right = vec3::Normalize(m_up.Cross(m_look)); }
		void SetRight(const vec3& _right) { m_right = _right; }

		const vec3& GetVelocity() const { return m_vel; }
		void SetVelocity(const vec3& _v) { m_vel = _v; }

		int GetMatchNum() const { return m_matchNum; }
		int GetCurNode() const { return m_curNode; }
		void SetMatchNum(const int _num) { m_matchNum = _num; }
		void SetCurNode(int _num) { m_curNode = _num; }

		virtual void Move(float _elapsedTime) {};
		virtual void UpdateBoundingBox();

	protected:
		vec3 m_vel = {};
		float m_maxVelXZ = 0.0f;
		float m_maxVelY = 0.0f;
		float m_friction = 0.0f;

		char m_dir = 0;
		vec3 m_look = {};
		vec3 m_up = {};
		vec3 m_right = {};

		int m_matchNum = -1;
		int m_curNode = -1;
	};

	class CTowerAttack : public CMoveObject
	{
	public:
		CTowerAttack();
		virtual ~CTowerAttack();

		bool Update(float _elapsedTime);

		void SetTarget(int _id) { m_targetID = _id; }

		std::atomic_bool m_active;

	private:
		void UpdateBoundingBox() override;

	private:
		int m_targetID;
	};

	class CStaticObject : public CGameObject
	{
	public:
		CStaticObject() {};
		virtual ~CStaticObject() {};

		int GetMaxHp() const { return m_maxHp; }
		int GetCurHp() { m_hpLock.lock(); int temp = m_curHp; m_hpLock.unlock(); return temp; }

		virtual void Reset() {}

	protected:
		int m_matchNum;
		int m_curHp;
		int m_maxHp;
		std::mutex m_hpLock;
	};

	class CTower : public CStaticObject
	{
	public:
		CTower(int _matchNum, int _id);
		virtual ~CTower();

		void Update(int _matchNum);
		void Damage(int _damage);

		void SetTargetID(int _id) { m_targetID = _id; }
		int GetTargetID() const { return m_targetID; }
		bool GetBroken() const { return m_broken; }

		std::atomic_bool m_active;

		virtual void Reset();

	private:
		int m_targetID;
		std::chrono::system_clock::time_point m_lastAttackTime;
		bool m_broken;
	};

	class CNexus : public CStaticObject
	{
	public:
		CNexus(int _matchNum);
		virtual ~CNexus() {};

		void Damage(int _damage);

		virtual void Reset();
	};
}