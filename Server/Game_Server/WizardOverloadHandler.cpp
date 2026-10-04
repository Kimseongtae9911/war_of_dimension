#include "pch.h"
#include "WizardOverloadHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {
    CSkillHandler* WizardOverloadHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new WizardOverloadHandler(client);
    }

    void WizardOverloadHandler::Handle()
    {
        m_client->GetStatus()->coolTimeBuff = COOLTIME_BUFF::OVERLOAD;
        auto buff = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardOverload)->buffInfo[EBuffType::Cooltime];

        network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(buff.buffDuration), EPlayerSkill::WizardOverload, {}, 0, 0, {}));
    }

}