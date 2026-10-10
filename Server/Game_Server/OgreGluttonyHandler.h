#pragma once
#include "CSkillHandler.h"

namespace wod_server {
	class OgreGluttonyHandler : public CSkillHandler
	{
	public:
		OgreGluttonyHandler() {}
		OgreGluttonyHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client) {}
		CSkillHandler* CreateHandler(std::shared_ptr<CClient> _client) override;

		void Handle() override;
	};

}