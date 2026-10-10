#include "pch.h"
#include "SwordManHeavySlashHandler.h"

namespace wod_server {

    SwordManHeavySlashHandler::SwordManHeavySlashHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::SwordManHeavySlash;
        SetSkillInfo();
    }

    CSkillHandler* SwordManHeavySlashHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManHeavySlashHandler(_client);
    }
}