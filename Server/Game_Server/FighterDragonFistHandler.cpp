#include "pch.h"
#include "FighterDragonFistHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterDragonFistHandler::FighterDragonFistHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterDragonFist;
        SetSkillInfo();
    }

    CSkillHandler* FighterDragonFistHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterDragonFistHandler(_client);
    }

    void FighterDragonFistHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT( m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), SkillDamage(), 0, SkillUseTime()));
    }

}