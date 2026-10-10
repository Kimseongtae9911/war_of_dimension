#include "pch.h"
#include "FighterSpinKickHandler.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"

namespace wod_server {
    FighterSpinKickHandler::FighterSpinKickHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::FighterSpinKick;
        SetSkillInfo();
    }

    CSkillHandler* FighterSpinKickHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterSpinKickHandler(_client);
    }

    void FighterSpinKickHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, m_client->GetLook(), SkillDamage(), 0, {}));
    }

}