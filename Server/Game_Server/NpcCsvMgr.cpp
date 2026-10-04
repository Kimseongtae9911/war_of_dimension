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

	void NpcCsvMgr::LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader)
	{
		tabledata::NpcInfo npcInfo;
		for (auto i = 2; i < datas.size(); ++i) {
			npcInfo = CreateStructFromCSV<tabledata::NpcInfo>(datas[i], csvHeader);
			m_npcCsvMap.emplace(npcInfo.Type, new NpcCsv(npcInfo));
		}
	}
}