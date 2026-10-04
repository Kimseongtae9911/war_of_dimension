#pragma once
#include "GameObject.h"

namespace wod_server {
	class CProtectedArea : public CStaticObject, public ISkillObject
	{
	public:
		CProtectedArea();
		~CProtectedArea() override;

		bool Update(float elapsedTime) override;

		void SetMatchNum(int matchNum) { m_matchNum = matchNum; }
		void SetDefensePower(int power) { m_defensePower = power; }

	private:
		int m_matchNum = -1;
		int m_defensePower = 0;

		std::unordered_set<int> m_area;
		std::mutex m_areaLock;
	};

}