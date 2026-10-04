#pragma once
#include "GameObject.h"

namespace wod_server {
	class CRockThrow : public CMoveObject, public ISkillObject
	{
	public:
		CRockThrow();
		~CRockThrow() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}


