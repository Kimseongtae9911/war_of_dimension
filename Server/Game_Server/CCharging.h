#pragma once
#include "GameObject.h"

namespace wod_server {
	class CCharging : public CMoveObject, public ISkillObject
	{
	public:
		CCharging();
		~CCharging() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
		void Reset();

		std::unordered_set<int> m_collideObjects;
	};
}