#pragma once
#include "GameObject.h"

namespace wod_server {
	class CBigBang : public CMoveObject, public ISkillObject
	{
	public:
		CBigBang();
		~CBigBang() override;

		bool Update(float _elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};

}