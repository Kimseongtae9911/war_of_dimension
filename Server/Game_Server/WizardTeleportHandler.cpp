#include "pch.h"
#include "WizardTeleportHandler.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"

namespace wod_server {
	CSkillHandler* WizardTeleportHandler::CreateHandler(std::shared_ptr<CClient> client)
	{
		return new WizardTeleportHandler(client);
	}

	void WizardTeleportHandler::Handle()
	{
		auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardTeleport);
		vec3 pos = m_client->GetPos();
		vec3 shift = m_client->GetLook() * skillCsv->skillRadius;

		while (true) {
			float height;
			int curNode;
			if (GameUtil::MapCollision(pos + shift, height, curNode,true)) {
				pos += shift;
				pos.y = height;
				m_client->SetCurNode(curNode);
				break;
			}
			else {
				shift = shift * 0.9f;
			}
		}

		//Skill End Event
		CNetworkMgr::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->castingTime), EPlayerSkill::WizardTeleport, pos, 0, 0, {}));
	}
}