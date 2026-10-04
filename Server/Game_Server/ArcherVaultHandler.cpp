#include "pch.h"
#include "ArcherVaultHandler.h"

namespace wod_server {
    ArcherVaultHandler::ArcherVaultHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::ArcherVault;
        SetSkillInfo();
    }

    CSkillHandler* ArcherVaultHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ArcherVaultHandler(client);
    }

    void ArcherVaultHandler::Handle()
    {
        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, SkillStartPos(), static_cast<int>(m_client->GetStatus()->GetStat().strength * 0.7f), 0, SkillUseTime()));
    }

}