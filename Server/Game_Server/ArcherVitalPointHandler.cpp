#include "pch.h"
#include "ArcherVitalPointHandler.h"

namespace wod_server {
    ArcherVitalPointHandler::ArcherVitalPointHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
    }

    CSkillHandler* ArcherVitalPointHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherVitalPointHandler(client);
    }

    void ArcherVitalPointHandler::Handle()
    {
        
    }

}