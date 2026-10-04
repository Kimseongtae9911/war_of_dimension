#include "pch.h"
#include "SwordManAttackHandler.h"
#include "GameUtil.h"

namespace wod_server {

    SwordManAttackHandler::SwordManAttackHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManAttack);
        m_strengthRatio = skillCsv->strengthRatio;
        m_startPosOffset = skillCsv->posOffset;
    }

    CSkillHandler* SwordManAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new SwordManAttackHandler(client);
    }

    void SwordManAttackHandler::Handle()
    {       
        DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(SkillStartPos(), { 0.f, 0.f, 0.f }, { m_client->GetBoundingBox().Extents }, { 2.f, 2.f, m_startPosOffset * 2.f }, m_client->GetWorldMatrix());

        GameUtil::HeroSkillCollisionCheck(box, m_client->GetMatchNum(), SkillDamage(), m_client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, m_client->GetID(), false);
    }

}