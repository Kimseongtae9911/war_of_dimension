#include "pch.h"
#include "ItemCsvMgr.h"

namespace wod_server {
	std::unique_ptr<ItemCsvMgr> ItemCsvMgr::m_instance;
	std::map<ITEMKIND, EItemType> ItemCsvMgr::ITEMKINDToEItemType = {
				{ITEMKIND::NONE, EItemType::None},
				{ITEMKIND::HEALHP, EItemType::HealHp},
				{ITEMKIND::HEALMP, EItemType::HealMp},
				{ITEMKIND::HP, EItemType::MaxHp},
				{ITEMKIND::MP, EItemType::MaxMp},
				{ITEMKIND::ATTACK, EItemType::StrengthIncrease},
				{ITEMKIND::MATTACK, EItemType::MagicIncrease},
				{ITEMKIND::DEFENSE, EItemType::DefenseIncrease},
				{ITEMKIND::MDEFENSE, EItemType::RegistIncrease},
				{ITEMKIND::SPEED, EItemType::SpeedIncrease},
				{ITEMKIND::TENACITY, EItemType::TenacityIncrease},
				{ITEMKIND::CRITICAL, EItemType::CriticalIncrease}
	};

	bool ItemCsvMgr::Initialize()
	{
		return true;
	}

	bool ItemCsvMgr::Release()
	{
		for (auto& [key, itemCsv] : m_itemCsvMap)
			delete itemCsv;

		return true;;
	}

	void ItemCsvMgr::LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader)
	{
		tabledata::ItemInfo itemInfo;
		for (auto i = 2; i < datas.size(); ++i) {
			itemInfo = CreateStructFromCSV<tabledata::ItemInfo>(datas[i], csvHeader);
			m_itemCsvMap.emplace(itemInfo.Type, new ItemCsv(itemInfo));
		}
	}
}