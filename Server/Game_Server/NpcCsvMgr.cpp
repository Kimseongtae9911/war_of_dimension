#include "pch.h"
#include "NpcCsvMgr.h"

namespace wod_server {
	std::unique_ptr<NpcCsvMgr> NpcCsvMgr::m_instance;

	bool NpcCsvMgr::Initialize()
	{
		return true;
	}

	bool NpcCsvMgr::Release()
	{
		for (auto& [key, npcCsv] : m_npcCsvMap)
			delete npcCsv;

		return true;
	}

	void NpcCsvMgr::LoadData(const TCsvData& _datas, const TCsvHeaderMap& _csvHeader)
	{
		tabledata::NpcInfo npcInfo;
		for (auto i = 2; i < _datas.size(); ++i) {
			npcInfo = CreateStructFromCSV<tabledata::NpcInfo>(_datas[i], _csvHeader);
			m_npcCsvMap.emplace(npcInfo.Type, new NpcCsv(npcInfo));
		}
	}
}