#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreDimensionCrushHandler : public CAttackSkillHandler
	{
	public:
		OgreDimensionCrushHandler() {}
		OgreDimensionCrushHandler(std::shared_ptr<CClient> client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;
	};

}