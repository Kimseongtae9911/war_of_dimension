#include "pch.h"
#include "SkillCsvMgr.h"

namespace wod_server {
	std::unique_ptr<SkillCsvMgr> SkillCsvMgr::m_instance;

	bool SkillCsvMgr::Initialize()
	{
		return true;
	}

	bool SkillCsvMgr::Release()
	{
		for (auto& [key, skillCsv] : m_skillCsvMap)
			delete skillCsv;

		return true;
	}

	void SkillCsvMgr::LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader)
	{
		tabledata::SkillInfo skillInfo;
		for (auto i = 2; i < _datas.size(); ++i) {
			skillInfo = CreateStructFromCSV<tabledata::SkillInfo>(_datas[i], _csvHeader);
			m_skillCsvMap.emplace(skillInfo.Type, new SkillCsv(skillInfo));
		}
	}
}