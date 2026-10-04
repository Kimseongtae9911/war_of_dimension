#include "pch.h"
#include "FighterMeditationHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* FighterMeditationHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        m_type = EPlayerSkill::FighterMeditation;
        return new FighterMeditationHandler(client);
    }

    void FighterMeditationHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::CurTime(), m_type, {}, 0, 0, {}));
    }

}