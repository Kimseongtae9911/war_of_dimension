#pragma once
#include "GameObject.h"

namespace wod_server {
	class CJudgementSword : public CMoveObject, public ISkillObject
	{
	public:
		CJudgementSword();
		~CJudgementSword() override;

		bool Update(float _elapsedTime) override;

		void SetTargetID(int _id) { m_targetID = _id; }

	private:
		void UpdateBoundingBox() override;

		int m_targetID = -1;
	};
}

