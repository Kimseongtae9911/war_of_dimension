#include "pch.h"
#include "WizardEnchantHandler.h"

namespace wod_server {
    CSkillHandler* WizardEnchantHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardEnchantHandler(client);
    }

    void WizardEnchantHandler::Handle()
    {
        std::cout << "Wizard WizardEnchantHandler" << std::endl;
    }

}