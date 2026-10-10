#pragma once

#include "GameObject.h"

namespace wod_server {
	class CReturnZero : public CMoveObject, public ISkillObject
	{
	public:
		CReturnZero();
		~CReturnZero() override;

		bool Update(float _elapsedTime) override;

		void SetStartPos(const vec3& _pos) { m_startPos = _pos; }

	private:
		void UpdateBoundingBox() override;

		vec3 m_startPos;
		bool m_return = false;
		std::unordered_set<int> m_collideIDs;
		DebuffInfo m_memoryLeak;
	};
}