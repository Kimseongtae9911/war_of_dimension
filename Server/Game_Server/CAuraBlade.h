#pragma once
#include "CClient.h"

namespace wod_server {
	class CAuraBlade : public CMoveObject, public ISkillObject
	{
	public:
		CAuraBlade();
		~CAuraBlade() override;

		bool Update(float elapsedTime) override;

	private:
		void UpdateBoundingBox() override;
		std::unordered_set<int> m_collideIDs;
	};

}