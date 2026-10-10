#include "pch.h"
#include "OgreCrunchHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    OgreCrunchHandler::OgreCrunchHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::OgreCrunch;
        SetSkillInfo();
    }

    CSkillHandler* OgreCrunchHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new OgreCrunchHandler(_client);
    }

    void OgreCrunchHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), SkillDamage(), 0, SkillUseTime()));
    }

}