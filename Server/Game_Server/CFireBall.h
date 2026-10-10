#pragma once
#include "GameObject.h"

namespace wod_server {
	class CFireBall : public CMoveObject, public ISkillObject
	{
	public:
		CFireBall();
		~CFireBall() override;

		bool Update(float _elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}