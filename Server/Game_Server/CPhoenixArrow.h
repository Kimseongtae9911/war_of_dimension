#pragma once
#include "GameObject.h"

namespace wod_server {
	class CPhoenixArrow : public CMoveObject, public ISkillObject
	{
	public:
		CPhoenixArrow();
		~CPhoenixArrow() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;

	private:
		DebuffInfo m_burnDebuff;
	};
}
