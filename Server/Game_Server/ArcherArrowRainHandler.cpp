#include "pch.h"
#include "ArcherArrowRainHandler.h"

namespace wod_server {
    ArcherArrowRainHandler::ArcherArrowRainHandler(std::shared_ptr<CClient> client) : CAttackSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherArrowRain;
    }

    CSkillHandler* ArcherArrowRainHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherArrowRainHandler(client);
    }
}
