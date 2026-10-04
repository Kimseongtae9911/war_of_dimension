#include "pch.h"
#include "WizardReflectHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* WizardReflectHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardReflectHandler(client);
    }

    void WizardReflectHandler::Handle()
    {
        m_client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::REFLECT;

        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardReflect);

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->buffInfo[EBuffType::Reflect].buffDuration), EPlayerSkill::WizardReflect, {}, 0, 0, {}));
    }

}