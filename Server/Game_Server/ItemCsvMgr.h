#pragma once

namespace wod_server {
    struct ItemCsv : tabledata::ItemInfo
    {
        ItemCsv(const ItemInfo& _itemInfo)
        {
            Type = _itemInfo.Type;
            Value = _itemInfo.Value;
            Time = _itemInfo.Time;
        }
    };

    class ItemCsvMgr : public CsvLoader, public TSingleton<ItemCsvMgr>
    {
    public:
        static std::map<ITEMKIND, EItemType> ITEMKINDToEItemType;   // 기존 클라이언트 코드와 대응될 수 있도록 임시로 사용
    public:
        bool Initialize() override;
        bool Release() override;

        void LoadData(const TCsvData& datas, const TCsvHeaderMap& csvHeader) override;
        ItemCsv* GetItemCsv(EItemType type) const {
            auto iter = m_itemCsvMap.find(type);
            if (iter == m_itemCsvMap.end()) {
                return nullptr;
            }
            return iter->second;
        }

    private:
        std::unordered_map<EItemType, ItemCsv*> m_itemCsvMap;
    };
}