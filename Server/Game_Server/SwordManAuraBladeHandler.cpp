#include "pch.h"
#include "SwordManAuraBladeHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    SwordManAuraBladeHandler::SwordManAuraBladeHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::SwordManAuraBlade;
        SetSkillInfo();
    }

    CSkillHandler* SwordManAuraBladeHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManAuraBladeHandler(client);
    }

}