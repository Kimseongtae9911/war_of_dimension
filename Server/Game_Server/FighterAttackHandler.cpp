#include "pch.h"
#include "FighterAttackHandler.h"
#include "GameUtil.h"

namespace wod_server {

    FighterAttackHandler::FighterAttackHandler(std::shared_ptr<CClient> client) : CSkillHandler(client)
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterAttack);
        m_strengthRatio = skillCsv->strengthRatio;
        m_startPosOffset = skillCsv->posOffset;
    }

    CSkillHandler* FighterAttackHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new FighterAttackHandler(client);
    }

    void FighterAttackHandler::Handle()
    {       
        try {
            DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(SkillStartPos(), m_client->GetLook(), {m_client->GetBoundingBox().Extents}, {2.0f, 1.5f, m_startPosOffset * 2.f}, m_client->GetWorldMatrix());

            GameUtil::HeroSkillCollisionCheck(box, m_client->GetMatchNum(), SkillDamage(), m_client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, m_client->GetID(), false);
        }
        catch (const std::exception& ex) {
            LogPrinter::PrintMsg("Err(FighterAttack Handle), " + std::string(ex.what()));
        }
    }

}