#include "pch.h"
#include "WizardBlinkHandler.h"
#include "GameUtil.h"
#include "CMatchMgr.h"
#include "CObjectMgr.h"

namespace wod_server {

	CSkillHandler* WizardBlinkHandler::CreateHandler(std::shared_ptr<CClient> client)
	{
		return new WizardBlinkHandler(client);
	}

	void WizardBlinkHandler::Handle()
	{
		auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardBlink);

		vec3 pos = m_client->GetPos();
		vec3 shift = m_client->GetLook() * skillCsv->skillRadius;

		while (true) {
			float height;
			int curNode;
			if (GameUtil::MapCollision(pos + shift, height, curNode, true)) {
				pos += shift;
				pos.y = height;
				m_client->SetPos(pos);
				m_client->SetCurNode(curNode);
				
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(m_client->GetMatchId(), 
						m_client->GetPos(), m_client->GetDir());
				}

				m_client->SetUsingSkill(false);
				break;
			}
			else {
				shift = shift * 0.9f;
			}
		}
	}

}