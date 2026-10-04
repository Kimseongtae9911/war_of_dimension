#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreGluttonyHandler : public CSkillHandler
	{
	public:
		OgreGluttonyHandler() {}
		OgreGluttonyHandler(std::shared_ptr<CClient> client) : CSkillHandler(client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> client) override;

		void Handle() override;
	};

}