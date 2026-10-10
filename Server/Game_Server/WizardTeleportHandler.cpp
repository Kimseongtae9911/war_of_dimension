#include "pch.h"
#include "WizardTeleportHandler.h"
#include "GameUtil.h"
#include "CNetworkMgr.h"

namespace wod_server {
	CSkillHandler* WizardTeleportHandler::CreateHandler(std::shared_ptr<CClient> _client)
	{
		return new WizardTeleportHandler(_client);
	}

	void WizardTeleportHandler::Handle()
	{
		auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardTeleport);
		vec3 pos = m_client->GetPos();
		vec3 shift = m_client->GetLook() * skillCsv->m_skillRadius;

		while (true) {
			float height;
			int curNode;
			if (GameUtil::MapCollision(pos + shift, height, curNode,true)) {
				pos += shift;
				pos.m_y = height;
				m_client->SetCurNode(curNode);
				break;
			}
			else {
				shift = shift * 0.9f;
			}
		}

		//Skill End Event
		CNetworkMgr::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_castingTime), EPlayerSkill::WizardTeleport, pos, 0, 0, {}));
	}
}