#include "pch.h"
#include "WizardOverloadHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* WizardOverloadHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new WizardOverloadHandler(_client);
    }

    void WizardOverloadHandler::Handle()
    {
        m_client->GetStatus()->m_coolTimeBuff = COOLTIME_BUFF::OVERLOAD;
        auto buff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardOverload)->m_buffInfo[EBuffType::Cooltime];

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(buff.m_buffDuration), EPlayerSkill::WizardOverload, {}, 0, 0, {}));
    }

}