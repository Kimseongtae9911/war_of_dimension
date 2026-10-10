#include "pch.h"
#include "OgreEndureHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* OgreEndureHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreEndureHandler(_client);
    }

    void OgreEndureHandler::Handle()
    {
        m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::ENDURE;
        auto castingTime = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreEndure)->m_castingTime;

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(castingTime), EPlayerSkill::OgreEndure, {}, 0, 0, {}));
    }

}