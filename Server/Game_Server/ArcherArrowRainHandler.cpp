#include "pch.h"
#include "ArcherArrowRainHandler.h"

namespace wod_server {
    ArcherArrowRainHandler::ArcherArrowRainHandler(std::shared_ptr<CClient> _client) : CAttackSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherArrowRain;
    }

    CSkillHandler* ArcherArrowRainHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherArrowRainHandler(_client);
    }
}
