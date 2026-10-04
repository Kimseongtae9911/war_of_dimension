#include "pch.h"
#include "ProReturnZeroHandler.h"

namespace wod_server {
    constexpr vec3 RETURN_ZERO_OFFSET = { 0.f, 0.9f, 0.f };

    CSkillHandler* ProReturnZeroHandler::CreateHandler(std::shared_ptr<CClient> client)
    {
        return new ProReturnZeroHandler(client);
    }

    void ProReturnZeroHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerReturn0);

        CStat stat = m_client->GetStatus()->GetStat();
        int objectID = CGameMgr::GetInstance()->ReturnZero(m_client->GetMatchNum(), m_client->GetPos() + m_client->GetLook() * skillCsv->posOffset + RETURN_ZERO_OFFSET, m_client->GetLook(), static_cast<int>(stat.magic * skillCsv->magicRatio), stat.critical, m_client->GetID());
        if (objectID == -1)
            return;
        for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
            if (-1 == id)
                continue;
            CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::PRO_RETURN_ZERO, m_client->GetPos() + m_client->GetLook() * skillCsv->posOffset + RETURN_ZERO_OFFSET, m_client->GetLook());
        }
    }

}