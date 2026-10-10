#include "pch.h"
#include "CItem.h"
#include "CClient.h"

namespace wod_server {

	bool CHealHpItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		m_owner->GetStatus()->m_healthMana.HealHp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->m_healthMana);
		}

		return true;
	}

	bool CHealMpItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		m_owner->GetStatus()->m_healthMana.HealMp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->m_healthMana);
		}

		return true;
	}

	bool CHpStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		auto status = m_owner->GetStatus();
		status->m_healthMana.SetMaxHp(status->m_healthMana.GetMaxHp() + itemCsv->Value);
		status->m_healthMana.HealHp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), m_owner->GetStatus()->m_healthMana);
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
		status->m_healthMana.SetMaxMp(status->m_healthMana.GetMaxMp() + itemCsv->Value);
		status->m_healthMana.HealMp(itemCsv->Value);

		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(m_owner->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(m_owner->GetMatchId(), status->m_healthMana);
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
		changeStat.m_strength = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CMagicStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.m_magic = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CArmorStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.m_armor = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CRegistStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.m_regist = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CSpeedStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.m_speed = itemCsv->Value;
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
		changeStat.m_endure = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

	bool CCriticalStatItem::Use()
	{
		auto itemCsv = ItemCsvMgr::GetInstance()->GetItemCsv(m_itemType);
		if (itemCsv == nullptr)
			return false;

		CStat changeStat(0);
		changeStat.m_critical = itemCsv->Value;
		m_owner->GetStatus()->GetStat().ChangeStatUntilRollback(m_owner->GetID(), changeStat, itemCsv->Time);

		return true;
	}

}