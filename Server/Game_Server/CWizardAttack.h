#pragma once
#include "GameObject.h"

namespace wod_server {
	class CWizardAttack : public CMoveObject, public ISkillObject
	{
	public:
		CWizardAttack();
		~CWizardAttack() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
	};
}