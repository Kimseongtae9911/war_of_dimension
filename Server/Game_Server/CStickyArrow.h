#pragma once
#include "GameObject.h"

namespace wod_server {
	class CStickyArrow : public CMoveObject, public ISkillObject
	{
	public:
		CStickyArrow();
		~CStickyArrow() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;

	private:
		DebuffInfo m_slowDebuff;
	};
}
