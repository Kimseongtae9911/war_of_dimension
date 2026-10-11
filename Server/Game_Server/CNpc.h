#pragma once

namespace wod_server {
	struct PathNode
	{
		Node* m_node;
		float m_gScore;
		float m_fScore;

		PathNode(Node* _node, float _gScore, float _fScore) : m_node(_node), m_gScore(_gScore), m_fScore(_fScore) {}
	};

	class CNpc : public CMoveObject
	{
	public:
		CNpc();
		virtual ~CNpc();

		ENpcType GetNpcType() const { return m_npcType;
        }

        NPC_TYPE GetPacketType() const
        {
            return m_npcType == ENpcType::None ? NPC_TYPE::MINION : static_cast<NPC_TYPE>(static_cast<int>(m_npcType) - 1);
        }

        virtual void Initialize(uint8_t _posIndex) = 0;

        void SetTargetPos(const vec2& _target) { m_targetPos = _target; }

		std::atomic_bool m_active = false;

		virtual void Respawn(int _currTime) = 0;
		virtual void Heal() = 0;

		virtual void Move(float _elapsedTime) override {}
		void Rotate(float _elapsedTime);
		virtual bool Damaged(int _clientID, int _power, DAMAGE_TYPE _type, bool _updateTarget = true);
		void UpdateBoundingBox() override;

		void LookTarget();

		void SetTargetClientID(int _id) { m_targetClientID = _id; }
		void SetState(NPC_STATE _state) { m_state = _state; }
		void SetAttack(bool _attack) { m_attack = _attack; }
		void InitializeHp(int _hp) { m_maxHp = _hp; m_curHp = _hp; }
		void SetMaxHp(int _hp) { m_maxHp = _hp; }
		void SetCurHp(int _hp) { m_hpLock.lock(); m_curHp = _hp; m_hpLock.unlock(); }
		void SetSpeed(float _speed) { m_speed = _speed; }
		void SetPower(int _power) { m_power = _power; }
		int GetMaxHp() const { return m_maxHp; }
		int GetCurHp() { m_hpLock.lock(); int temp = m_curHp; m_hpLock.unlock(); return temp; }
		float GetSpeed() const { return m_speed; }
		int GetPower() const { return m_power; }

		virtual void Reset() {}

	protected:
		virtual void ReturnToPath() {}
		virtual void Chase(float _elapedTime);
		void Attack(float _elapsedTimes);
		bool Astar(Node* _startNode, Node* _endNode);
		float CalculateHeuristic(Node* _node, Node* _endNode);

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
