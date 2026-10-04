#include "pch.h"
#include "CItemFactory.h"

namespace wod_server {

    std::unordered_map<EItemType, std::function<std::unique_ptr<CItem>(std::shared_ptr<CClient> _client)>> CItemFactory::m_itemFactory = {
    { EItemType::HealHp, [](std::shared_ptr<CClient> _client) {return std::make_unique<CHealHpItem>(_client, EItemType::HealHp); } },
    { EItemType::HealMp , [](std::shared_ptr<CClient> _client) {return std::make_unique<CHealMpItem>(_client, EItemType::HealMp); } },
    { EItemType::MaxHp, [](std::shared_ptr<CClient> _client) {return std::make_unique<CHpStatItem>(_client, EItemType::MaxHp); } },
    { EItemType::MaxMp, [](std::shared_ptr<CClient> _client) {return std::make_unique<CMpStatItem>(_client, EItemType::MaxMp); } },
    { EItemType::StrengthIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CStrengthStatItem>(_client, EItemType::StrengthIncrease); } },
    { EItemType::MagicIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CMagicStatItem>(_client, EItemType::MagicIncrease); } },
    { EItemType::DefenseIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CArmorStatItem>(_client, EItemType::DefenseIncrease); } },
    { EItemType::SpeedIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CSpeedStatItem>(_client, EItemType::SpeedIncrease); } },
    { EItemType::TenacityIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CEndureStatItem>(_client, EItemType::TenacityIncrease); } },
    { EItemType::CriticalIncrease, [](std::shared_ptr<CClient> _client) {return std::make_unique<CCriticalStatItem>(_client, EItemType::CriticalIncrease); } }
    };

    bool CItemFactory::UseItem(EItemType _itemType, std::shared_ptr<CClient> _client)
	{
        auto it = m_itemFactory.find(_itemType);
        if (it == m_itemFactory.end())
            return false;

        return it->second(_client)->Use();
	}
}