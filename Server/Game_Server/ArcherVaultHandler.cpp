#include "pch.h"
#include "ArcherVaultHandler.h"

namespace wod_server {
    ArcherVaultHandler::ArcherVaultHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        m_type = EPlayerSkill::ArcherVault;
        SetSkillInfo();
    }

    CSkillHandler* ArcherVaultHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ArcherVaultHandler(_client);
    }

    void ArcherVaultHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, SkillStartPos(), static_cast<int>(m_client->GetStatus()->GetStat().m_strength * 0.7f), 0, SkillUseTime()));
    }

}