#pragma once
#include <ServerCore/GameObject.h>
#include <DirectXCollision.h>
#include "MathUtil.h"

namespace wod_server
{
struct GameObjectGeometry
{
    using Vector3 = vec3;
    using Vector2 = vec2;
    using BoundingBox = DirectX::BoundingOrientedBox;
    using Matrix = DirectX::XMFLOAT4X4;

    static void UpdateBoundingBox(const Vector3& _right, const Vector3& _look, const Vector3& _pos, Matrix& _world, const BoundingBox& _initial, BoundingBox& _result);
};

using CGameObject = wod::core::GameObject<GameObjectGeometry>;
using CMoveObject = wod::core::MoveObject<GameObjectGeometry>;

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