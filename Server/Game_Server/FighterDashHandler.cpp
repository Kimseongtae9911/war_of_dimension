#include "pch.h"
#include "FighterDashHandler.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"
#include "GameUtil.h"

namespace wod_server {
    CSkillHandler* FighterDashHandler::CreateHandler(std::shared_ptr<CClient> _client)
    {
        return new FighterDashHandler(_client);
    }

    void FighterDashHandler::Handle()
    {
		auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterDash);
		vec3 pos = m_client->GetPos();
		vec3 shift = m_client->GetLook() * skillCsv->m_skillRadius;

		while (true) {
			float height;
			int curNode;
			if (GameUtil::MapCollision(pos + shift, height, curNode, true)) {
				pos += shift;
				pos.m_y = height;
				m_client->SetPos(pos);
				m_client->SetCurNode(curNode);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(m_client->GetMatchId(), m_client->GetPos(), m_client->GetDir());
				}

				break;
			}
			else {
				shift = shift * 0.9f;
			}
		}
    }

}