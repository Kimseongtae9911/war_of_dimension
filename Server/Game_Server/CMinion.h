#pragma once

namespace wod_server {
	class CMinion : public CNpc
	{
	public:
		CMinion();
		~CMinion();

		void Initialize(uint8_t _posIndex) override;
		void SetPath(int path) { m_path = path; m_pathCount = 0; }

		bool Update(float elapsedTime) override;
		void Move(float elapsedTime) override;
		void Reset() override;
		void Respawn(int _currTime) override;
		void Heal() override {}

		bool AttackStructure();

	private:
		void ReturnToPath() override;
		void MoveToStructure(float elapsedTime);

	private:
		int m_path;
		int m_pathCount;
	};
}
