#pragma once

#include "GameObject.h"

namespace wod_server {
	class CReturnZero : public CMoveObject, public ISkillObject
	{
	public:
		CReturnZero();
		~CReturnZero() override;

		bool Update(float elapsedTime) override;

		void SetStartPos(const vec3& pos) { m_startPos = pos; }

	private:
		void UpdateBoundingBox() override;

		vec3 m_startPos;
		bool m_return = false;
		std::unordered_set<int> m_collideIDs;
		DebuffInfo m_memoryLeak;
	};
}