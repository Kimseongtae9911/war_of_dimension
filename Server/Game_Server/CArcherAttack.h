#pragma once
#include "GameObject.h"

namespace wod_server {
	class CArcherAttack : public CMoveObject, public ISkillObject
	{
	public:
		CArcherAttack();
		~CArcherAttack() override;

		bool Update(float _elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}
