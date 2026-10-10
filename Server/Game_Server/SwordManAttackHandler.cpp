#include "pch.h"
#include "SwordManAttackHandler.h"
#include "GameUtil.h"

namespace wod_server {

    SwordManAttackHandler::SwordManAttackHandler(std::shared_ptr<CClient> _client) : CSkillHandler(_client)
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManAttack);
        m_strengthRatio = skillCsv->m_strengthRatio;
        m_startPosOffset = skillCsv->m_posOffset;
    }

    CSkillHandler* SwordManAttackHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new SwordManAttackHandler(_client);
    }

    void SwordManAttackHandler::Handle()
    {
        DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(SkillStartPos(), { 0.f, 0.f, 0.f }, { m_client->GetBoundingBox().Extents }, { 2.f, 2.f, m_startPosOffset * 2.f }, m_client->GetWorldMatrix());

        GameUtil::HeroSkillCollisionCheck(box, m_client->GetMatchNum(), SkillDamage(), m_client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, m_client->GetID(), false);
    }

}