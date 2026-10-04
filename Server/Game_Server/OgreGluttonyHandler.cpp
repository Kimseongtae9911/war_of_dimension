#include "pch.h"
#include "OgreGluttonyHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* OgreGluttonyHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new OgreGluttonyHandler(client);
    }

    void OgreGluttonyHandler::Handle()
    {
        m_client->GetStatus()->damageBuff = DAMAGE_BUFF::GLUTTONY;
        auto castingTime = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreGluttony)->castingTime;

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(castingTime), EPlayerSkill::OgreGluttony, {}, 0, 0, {}));
    }

}