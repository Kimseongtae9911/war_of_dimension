#include "pch.h"
#include "SwordManJudgementSwordHandler.h"
#include "CNetworkMgr.h"
#include "GameUtil.h"

namespace wod_server {
    SwordManJudgementSwordHandler::SwordManJudgementSwordHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        m_type = EPlayerSkill::SwordManJudgementSword;
        SetSkillInfo();
    }

    CSkillHandler* SwordManJudgementSwordHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManJudgementSwordHandler(client);
    }

    void SwordManJudgementSwordHandler::Handle()
    {
		network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, m_client->GetPos(), SkillDamage(), 0, {}));
    }

}