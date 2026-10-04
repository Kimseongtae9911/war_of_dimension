#include "pch.h"
#include "SwordManHellBladeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    SwordManHellBladeHandler::SwordManHellBladeHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::SwordManHellBlade;
        SetSkillInfo();
    }

    CSkillHandler* SwordManHellBladeHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManHellBladeHandler(client);
    }

    void SwordManHellBladeHandler::Handle()
    {
        CNetworkMgr::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, {}, SkillDamage(), 0, {}));
    }

}