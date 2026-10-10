#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreChargingHandler : public CSkillHandler
	{
	public:
		OgreChargingHandler() {}
		OgreChargingHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}