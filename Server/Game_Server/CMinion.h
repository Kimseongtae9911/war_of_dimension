#pragma once

namespace wod_server {
	class CMinion : public CNpc
	{
	public:
		CMinion();
		~CMinion();

		void Initialize(uint8_t _posIndex) override;
		void SetPath(int _path) { m_path = _path; m_pathCount = 0; }

		bool Update(float _elapsedTime) override;
		void Move(float _elapsedTime) override;
		void Reset() override;
		void Respawn(int _currTime) override;
		void Heal() override {}

		bool AttackStructure();

	private:
		void ReturnToPath() override;
		void MoveToStructure(float _elapsedTime);

	private:
		int m_path;
		int m_pathCount;
	};
}
