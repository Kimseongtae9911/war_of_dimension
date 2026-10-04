#include "pch.h"
#include "CItem.h"
#include "CClient.h"

namespace wod_server {

	bool CHealHpItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		m_owner->GetStatus()->healthMana.HealHp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->healthMana);
		}

		return true;
	}

	bool CHealMpItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		m_owner->GetStatus()->healthMana.HealMp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->healthMana);
		}

		return true;
	}

	bool CHpStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		auto status = m_owner->GetStatus();
		status->healthMana.SetMaxHp(status->healthMana.GetMaxHp() + itemCsv->Value);
		status->healthMana.HealHp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->healthMana);
		}
		network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(m_owner->GetID(), TimeUtil::PassedTimeMSec(itemCsv->Time), EVENT_TYPE::EV_HEALTHMANA_CHANGE, -1, {}, itemCsv->Value, 0));

		return true;
	}

	bool CMpStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		auto status = m_owner->GetStatus();
		status->healthMana.SetMaxMp(status->healthMana.GetMaxMp() + itemCsv->Value);
		status->healthMana.HealMp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), status->healthMana);
		}
		network::GetInstance()->RegisterTimerEvent(TIMER_EVENT(m_owner->GetID(), TimeUtil::PassedTimeMSec(itemCsv->Time), EVENT_TYPE::EV_HEALTHMANA_CHANGE, -1, {}, 0, itemCsv->Value));

		return true;
	}

	bool CStrengthStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.strength = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CMagicStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.magic = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CArmorStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.armor = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CRegistStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.regist = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CSpeedStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.speed = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatChangePacket(m_owner->GetMatchId(), m_owner->GetStatus()->GetStat());
		}

		return true;
	}

	bool CEndureStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.endure = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CCriticalStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.critical = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

}