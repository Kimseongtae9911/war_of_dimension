#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreChargingHandler : public CSkillHandler
	{
	public:
		OgreChargingHandler() {}
		OgreChargingHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}