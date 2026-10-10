#include "pch.h"
#include "WizardEnchantHandler.h"

namespace wod_server {
    CSkillHandler* WizardEnchantHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardEnchantHandler(_client);
    }

    void WizardEnchantHandler::Handle()
    {
        std::cout << "Wizard WizardEnchantHandler" << std::endl;
    }

}