#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreDimensionCrushHandler : public CAttackSkillHandler
	{
	public:
		OgreDimensionCrushHandler() {}
		OgreDimensionCrushHandler(std::shared_ptr<CClient> _client);
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;
	};

}