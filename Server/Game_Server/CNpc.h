#pragma once

namespace wod_server {
	struct PathNode
	{
		Node* node;
		float gScore;
		float fScore;

		PathNode(Node* node, float gScore, float fScore) : node(node), gScore(gScore), fScore(fScore) {}
	};

	class CNpc : public CMoveObject
	{
	public:
		CNpc();
		virtual ~CNpc();

		ENpcType GetNpcType() const { return m_npcType; }
		virtual void Initialize(uint8_t _posIndex) = 0;

		void SetTargetPos(const vec2& target) { m_targetPos = target; }

		std::atomic_bool active = false;

		virtual void Respawn(int _currTime) = 0;
		virtual void Heal() = 0;

		virtual void Move(float elapsedTime) override {}
		void Rotate(float elapsedTime);
		virtual bool Damaged(int clientID, int power, DAMAGE_TYPE type, bool updateTarget = true);
		void UpdateBoundingBox() override;

		void LookTarget();

		void SetTargetClientID(int id) { m_targetClientID = id; }
		void SetState(NPC_STATE state) { m_state = state; }
		void SetAttack(bool attack) { m_attack = attack; }
		void InitializeHp(int _hp) { m_maxHp = _hp; m_curHp = _hp; }
		void SetMaxHp(int hp) { m_maxHp = hp; }
		void SetCurHp(int hp) { m_hpLock.lock(); m_curHp = hp; m_hpLock.unlock(); }
		void SetSpeed(float speed) { m_speed = speed; }		
		void SetPower(int power) { m_power = power; }
		int GetMaxHp() const { return m_maxHp; }
		int GetCurHp() { m_hpLock.lock(); int temp = m_curHp; m_hpLock.unlock(); return temp; }
		float GetSpeed() const { return m_speed; }
		int GetPower() const { return m_power; }

		virtual void Reset() {}

	protected:
		virtual void ReturnToPath() {}
		virtual void Chase(float elapedTime);
		void Attack(float elapsedTimes);
		bool Astar(Node* startNode, Node* endNode);
		float CalculateHeuristic(Node* node, Node* endNode);

	protected:
		std::stack<Node*> m_chasePath;
		int m_targetClientID;
		vec2 m_targetPos;
		vec3 m_targetLook;
		bool m_rotate = false;
		bool m_attack = false;

		NPC_STATE m_state = NPC_STATE::ST_IDLE;
		std::chrono::system_clock::duration m_attackTime;

		int m_maxHp;
		int m_curHp;
		std::mutex m_hpLock;

		float m_speed;
		uint16_t m_rotateSpeed = 0;
		int m_power;

		ENpcType m_npcType = ENpcType::None;
		NpcCsv* m_npcCsv = nullptr;
	};
}