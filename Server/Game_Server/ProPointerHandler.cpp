#include "pch.h"
#include "ProPointerHandler.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"
#include "CObjectMgr.h"
#include "CMatchMgr.h"

namespace wod_server {
    CSkillHandler* ProPointerHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new ProPointerHandler(_client);
    }

    void ProPointerHandler::Handle()
    {
        auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPointer);

        std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum());
        float distance = FLT_MAX;
        int targetID = -1;
        for (int i = 0; i < MAX_PLAYER - 1; ++i) {
            if (clientIDs[i] == -1)
                continue;
            float temp = DistanceXZ(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos(), m_client->GetPos());
            if (temp < distance) {
                distance = temp;
                targetID = clientIDs[i];
            }
        }

        if(targetID != -1)
            network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EPlayerSkill::ProgrammerPointer, {}, 0, 0, {}, targetID));
    }

}