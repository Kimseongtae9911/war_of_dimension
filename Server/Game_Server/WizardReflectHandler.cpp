#include "pch.h"
#include "WizardReflectHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* WizardReflectHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardReflectHandler(_client);
    }

    void WizardReflectHandler::Handle()
    {
        m_client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::REFLECT;

        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardReflect);

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_buffInfo[EBuffType::Reflect].m_buffDuration), EPlayerSkill::WizardReflect, {}, 0, 0, {}));
    }

}