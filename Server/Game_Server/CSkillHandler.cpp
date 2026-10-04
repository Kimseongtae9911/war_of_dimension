#include "pch.h"
#include "CSkillHandler.h"
#include "CNetworkMgr.h"

namespace wod_server {

	int CSkillHandler::SkillDamage()
	{
		CStat stat = m_client->GetStatus()->GetStat();
		return static_cast<int>(stat.strength * m_strengthRatio + stat.magic * m_magicRatio);
	}

	void CSkillHandler::SetSkillInfo()
	{
		const auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(m_type);
		if(skillCsv == nullptr)
		{
			LogPrinter::PrintMsg("SkillCsv is null");
			return;
		}
		m_strengthRatio = skillCsv->strengthRatio;
		m_castingTime = skillCsv->castingTime;
		m_startPosOffset = skillCsv->posOffset;
	}

	void CAttackSkillHandler::Handle()
	{
		network::GetInstance()->RegisterSkillEvent(SKILL_EVENT(m_client->GetID(), SkillUseTime(), m_type, SkillStartPos(), SkillDamage(), 0, {}));
	}
	
}