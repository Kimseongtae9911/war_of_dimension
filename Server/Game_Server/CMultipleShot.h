#pragma once
#include "GameObject.h"

namespace wod_server {
	class CMultipleShot : public CMoveObject, public ISkillObject
	{
	public:
		CMultipleShot();
		~CMultipleShot() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}
