#pragma once

#include "CItem.h"

namespace wod_server {

	class CItemFactory
	{
	public:
		static bool UseItem(EItemType _itemType, std::shared_ptr<CClient> _client);

	private:
		static std::unordered_map<EItemType, std::function<std::unique_ptr<CItem>(std::shared_ptr<CClient> client)>> m_itemFactory;
	};

}