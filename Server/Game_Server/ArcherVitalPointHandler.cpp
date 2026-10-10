#include "pch.h"
#include "ArcherVitalPointHandler.h"

namespace wod_server {
    ArcherVitalPointHandler::ArcherVitalPointHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
    }

    CSkillHandler* ArcherVitalPointHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherVitalPointHandler(_client);
    }

    void ArcherVitalPointHandler::Handle()
    {

    }

}