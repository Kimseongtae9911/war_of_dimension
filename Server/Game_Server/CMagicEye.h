#pragma once
#include "GameObject.h"

namespace wod_server {
	class CMagicEye : public CMoveObject, public ISkillObject
	{
	public:
		CMagicEye();
		~CMagicEye() override;

		bool Update(float _elapsedTime) override;
		void ResetCheckTime() { m_lastCheckTime = TimeUtil::CurTime(); }

	private:
		std::chrono::system_clock::time_point m_lastCheckTime;
	};
}
