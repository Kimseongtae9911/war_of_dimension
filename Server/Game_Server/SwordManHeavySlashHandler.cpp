#include "pch.h"
#include "SwordManHeavySlashHandler.h"

namespace wod_server {

    SwordManHeavySlashHandler::SwordManHeavySlashHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::SwordManHeavySlash;
        SetSkillInfo();
    }

    CSkillHandler* SwordManHeavySlashHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManHeavySlashHandler(client);
    }
}