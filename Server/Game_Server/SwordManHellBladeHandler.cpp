#include "pch.h"
#include "SwordManHellBladeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    SwordManHellBladeHandler::SwordManHellBladeHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::SwordManHellBlade;
        SetSkillInfo();
    }

    CSkillHandler* SwordManHellBladeHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManHellBladeHandler(_client);
    }

    void SwordManHellBladeHandler::Handle()
    {
        CNetworkMgr::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, {}, SkillDamage(), 0, {}));
    }

}