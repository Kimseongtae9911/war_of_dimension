#pragma once
#include "GameObject.h"

namespace wod_server {
	class CMagicMissile : public CMoveObject, public ISkillObject
	{
	public:
		CMagicMissile();
		~CMagicMissile() override;

		bool Update(float _elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}