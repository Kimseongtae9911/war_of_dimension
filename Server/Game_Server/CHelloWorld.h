#pragma once
#include "GameObject.h"

namespace wod_server {
	class CHelloWorld : public CStaticObject, public ISkillObject
	{
	public:
		CHelloWorld();
		~CHelloWorld() override;

		bool Update(float _elapsedTime) override;

		void SetMatchNum(int _matchNum) { m_matchNum = _matchNum; }
		void SetArea(const std::vector<int>& _ids);

	private:
		int m_matchNum = -1;
		std::unordered_set<int> m_area;
	};

}