#pragma once
#include "GameObject.h"

namespace wod_server {
	class CProtectedArea : public CStaticObject, public ISkillObject
	{
	public:
		CProtectedArea();
		~CProtectedArea() override;

		bool Update(float _elapsedTime) override;

		void SetMatchNum(int _matchNum) { m_matchNum = _matchNum; }
		void SetDefensePower(int _power) { m_defensePower = _power; }

	private:
		int m_matchNum = -1;
		int m_defensePower = 0;

		std::unordered_set<int> m_area;
		std::mutex m_areaLock;
	};

}