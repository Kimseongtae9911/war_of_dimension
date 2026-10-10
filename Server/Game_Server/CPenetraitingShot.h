#pragma once
#include "GameObject.h"

namespace wod_server {
	class CPenetraitingShot : public CMoveObject, public ISkillObject
	{
	public:
		CPenetraitingShot();
		~CPenetraitingShot() override;

		bool Update(float _elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}
