#pragma once
#include "GameObject.h"

namespace wod_server {
	class CJudgementSword : public CMoveObject, public ISkillObject
	{
	public:
		CJudgementSword();
		~CJudgementSword() override;

		bool Update(float elapsedTime) override;

		void SetTargetID(int id) { m_targetID = id; }

	private:
		void UpdateBoundingBox() override;

		int m_targetID = -1;
	};
}

