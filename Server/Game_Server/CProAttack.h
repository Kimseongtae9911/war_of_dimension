#pragma once
#include "GameObject.h"

namespace wod_server {
	class CProAttack : public CMoveObject, public ISkillObject
	{
	public:
		CProAttack();
		~CProAttack() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}