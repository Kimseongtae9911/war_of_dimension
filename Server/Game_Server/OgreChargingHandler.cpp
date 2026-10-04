#include "pch.h"
#include "OgreChargingHandler.h"

namespace wod_server {
    CSkillHandler* OgreChargingHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreChargingHandler(client);
    }

    void OgreChargingHandler::Handle()
    {
        CGameMgr::GetInstance()->OgreCharging(m_client->GetMatchNum(), m_client->GetID());
    }

}