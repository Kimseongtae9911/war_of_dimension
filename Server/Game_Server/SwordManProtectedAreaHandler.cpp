#include "pch.h"
#include "SwordManProtectedAreaHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    SwordManProtectedAreaHandler::SwordManProtectedAreaHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::SwordManProtectedArea;
        SetSkillInfo();
        m_armorRatio = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManProtectedArea)->extraParam1;
    }

    CSkillHandler* SwordManProtectedAreaHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManProtectedAreaHandler(client);
    }

    void SwordManProtectedAreaHandler::Handle()
    {        
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, SkillStartPos(), static_cast<int>(m_client->GetStatus()->GetStat().armor * m_armorRatio), 0, {}));
    }
}