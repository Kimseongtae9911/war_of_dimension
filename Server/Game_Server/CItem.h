#pragma once
#include "CStat.h"

namespace wod_server {

	class CClient;

	class CItemInfo
	{
	public:
		CItemInfo() { m_type = ITEMKIND::NONE; };
		~CItemInfo() {};

		std::atomic_bool exist = false;

		void SetType(ITEMKIND type) { m_type = type; }
		ITEMKIND GetType() const { return m_type; }

	private:
		ITEMKIND m_type;
	};

	class CItem : public IItem
	{
	public:
		CItem() {}
		CItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) { m_owner = _client; m_itemType = _itemType; }
		~CItem() {}

	protected:
		std::shared_ptr<CClient> m_owner = nullptr;
		EItemType m_itemType = EItemType::None;
	};

	class CHealHpItem : public CItem
	{
	public:
		CHealHpItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CHealMpItem : public CItem
	{
	public:
		CHealMpItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CHpStatItem : public CItem
	{
	public:
		CHpStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CMpStatItem : public CItem
	{
	public:
		CMpStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CStrengthStatItem : public CItem
	{
	public:
		CStrengthStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CMagicStatItem : public CItem
	{
	public:
		CMagicStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CArmorStatItem : public CItem
	{
	public:
		CArmorStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CRegistStatItem : public CItem
	{
	public:
		CRegistStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CSpeedStatItem : public CItem
	{
	public:
		CSpeedStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CEndureStatItem : public CItem
	{
	public:
		CEndureStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

	class CCriticalStatItem : public CItem
	{
	public:
		CCriticalStatItem(std::shared_ptr<CClient> _client, EItemType _itemType = EItemType::None) : CItem(_client, _itemType) {}

		bool Use() override;
	};

}