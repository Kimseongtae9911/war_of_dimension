#pragma once
#include "GameObject.h"

namespace wod_server {
	class CDimensionCrush : public CStaticObject, public ISkillObject
	{
	public:
		CDimensionCrush();
		~CDimensionCrush() override;

		bool Update(float elapsedTime) override;

		void SetMatchNum(int matchNum) { m_matchNum = matchNum; }

	private:
		int m_matchNum = -1;
		std::unordered_set<int> m_area;
		std::mutex m_areaLock;
		std::unordered_map<int, std::chrono::system_clock::time_point> m_lastDamageTime;
	};

}