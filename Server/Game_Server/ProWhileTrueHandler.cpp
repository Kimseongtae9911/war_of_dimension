#include "pch.h"
#include "ProWhileTrueHandler.h"

namespace wod_server {
    CSkillHandler* ProWhileTrueHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProWhileTrueHandler(_client);
    }

    void ProWhileTrueHandler::Handle()
    {
        if (CGameMgr::GetInstance()->GetWhileTrue(m_client->GetMatchNum())) {
            CGameMgr::GetInstance()->WhileTrue(false, m_client->GetMatchNum());
        }
        else {
            auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerWhileTrue);
            CGameMgr::GetInstance()->WhileTrue(true, m_client->GetMatchNum());
            network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::CurTime(), EPlayerSkill::ProgrammerWhileTrue, {}, static_cast<int>(m_client->GetStatus()->GetStat().m_magic * skillCsv->m_magicRatio), 0, TimeUtil::CurTime()));
        }
    }

}