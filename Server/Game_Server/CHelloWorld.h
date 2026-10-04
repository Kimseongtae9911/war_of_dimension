#pragma once
#include "GameObject.h"

namespace wod_server {
	class CHelloWorld : public CStaticObject, public ISkillObject
	{
	public:
		CHelloWorld();
		~CHelloWorld() override;

		bool Update(float elapsedTime) override;

		void SetMatchNum(int matchNum) { m_matchNum = matchNum; }
		void SetArea(const std::vector<int>& ids);

	private:
		int m_matchNum = -1;
		std::unordered_set<int> m_area;
	};

}