#pragma once

#include "GameObject.h"

namespace wod_server {
	class CEnergyBall : public CMoveObject, public ISkillObject
	{
	public:
		CEnergyBall();
		~CEnergyBall() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}