#include "pch.h"
#include "FighterIndestructibleHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

    FighterIndestructibleHandler::FighterIndestructibleHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterIndestructible;
        SetSkillInfo();
    }

    CSkillHandler* FighterIndestructibleHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterIndestructibleHandler(_client);
    }

    void FighterIndestructibleHandler::Handle()
    {
        if (m_client->GetStatus()->m_defensiveBuff == DEFENSIVE_BUFF::INDESTRUCTIBLE) {
            m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;
            m_client->SetUsingSkill(false);
        }
        else {
            m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::INDESTRUCTIBLE;
            network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, {}, 0, 0, {}));
        }
    }

}