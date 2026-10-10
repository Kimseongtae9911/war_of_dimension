#include "pch.h"
#include "OgreGluttonyHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* OgreGluttonyHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreGluttonyHandler(_client);
    }

    void OgreGluttonyHandler::Handle()
    {
        m_client->GetStatus()->m_damageBuff = DAMAGE_BUFF::GLUTTONY;
        auto castingTime = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreGluttony)->m_castingTime;

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(castingTime), EPlayerSkill::OgreGluttony, {}, 0, 0, {}));
    }

}