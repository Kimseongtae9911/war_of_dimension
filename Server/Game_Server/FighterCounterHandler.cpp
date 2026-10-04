#include "pch.h"
#include "FighterCounterHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    FighterCounterHandler::FighterCounterHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {        
        m_type = EPlayerSkill::FighterCounter;
        SetSkillInfo();
    }

    CSkillHandler* FighterCounterHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterCounterHandler(client);
    }

    void FighterCounterHandler::Handle()
    {
        if (m_client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::INDESTRUCTIBLE) {
            return;
        }
        else if (m_client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::COUNTER) {
            m_client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;
        }
        else {
            m_client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::COUNTER;
            network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, {}, 0, 0, {}));
        }
    }
}