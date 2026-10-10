#include "pch.h"
#include "OgreChargingHandler.h"

namespace wod_server {
    CSkillHandler* OgreChargingHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreChargingHandler(_client);
    }

    void OgreChargingHandler::Handle()
    {
        CGameMgr::GetInstance()->OgreCharging(m_client->GetMatchNum(), m_client->GetID());
    }

}