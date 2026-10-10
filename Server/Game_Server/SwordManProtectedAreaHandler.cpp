#include "pch.h"
#include "SwordManProtectedAreaHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    SwordManProtectedAreaHandler::SwordManProtectedAreaHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::SwordManProtectedArea;
        SetSkillInfo();
        m_armorRatio = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManProtectedArea)->m_extraParam1;
    }

    CSkillHandler* SwordManProtectedAreaHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManProtectedAreaHandler(_client);
    }

    void SwordManProtectedAreaHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, SkillStartPos(), static_cast<int>(m_client->GetStatus()->GetStat().m_armor * m_armorRatio), 0, {}));
    }
}