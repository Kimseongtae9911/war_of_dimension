#include "pch.h"
#include "CArcherTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CArcherTimer::Attack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherAttack);

		const auto stat = client->GetStatus()->GetStat();
		const auto objectPos = skillInfo->GetStartPos(client->GetPos(), client->GetLook()) + ClientInfos::HERO_ATTACK_OFFSET;
		const auto damage = skillInfo->GetDamage(stat.m_strength, stat.m_magic);
		int objectID = CGameMgr::GetInstance()->ArcherAttack(client->GetMatchNum(), objectPos, client->GetLook(), damage, stat.m_critical, _ev.m_objID);
		if (objectID == -1)
			return;
		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_ATTACK, objectPos, client->GetLook());
		}
	}

	void CArcherTimer::Vault(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherVault);

		const auto stat = client->GetStatus()->GetStat();
		const auto objectPos = skillInfo->GetStartPos(client->GetPos(), client->GetLook());
		const auto damage = skillInfo->GetDamage(stat.m_strength, stat.m_magic);
		if (_ev.m_repeatTime == 0) {

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(objectPos, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 1.f, 3.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), damage, stat.m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);

			if (client->GetUsingSkill())
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherVault, client->GetLook(), 0, _ev.m_repeatTime + 1, TimeUtil::CurTime() });
		}
		else {
			//ev.pos = look
			vec3 newPos = client->GetPos() - _ev.m_pos * skillInfo->m_speed * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

			float height;
			int curNode;
			if (GameUtil::MapCollision(newPos, height, curNode)) {
				newPos.m_y = height;
				client->SetPos(newPos);
				client->SetCurNode(curNode);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}
			}

			if (_ev.m_repeatTime < skillInfo->m_repeatTime)
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherVault, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, TimeUtil::CurTime() });
		}
	}

	void CArcherTimer::BackStep(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherBackStep);

		vec3 newPos = client->GetPos() + _ev.m_pos * (skillInfo->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.m_y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
			}
		}

		if (client->GetUsingSkill())
			_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherBackStep, _ev.m_pos, 0, 0, TimeUtil::CurTime() });
	}

	void CArcherTimer::Dodge(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherDodge);

		vec3 newPos = client->GetPos() + _ev.m_pos * (skillInfo->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.m_y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
			}
		}

		if (client->GetUsingSkill())
			_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherDodge, _ev.m_pos, 0, 0, TimeUtil::CurTime() });
	}

	void CArcherTimer::PenetraitingShot(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->PenetraitingShot(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_PENETRAITING_SHOT, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer PenetraitingShot), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::StickyArrow(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->StickyArrow(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_STICKY_ARROW, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer StickyArrow), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::PhoenixArrow(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->PhoenixArrow(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_PHOENIX_ARROW, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer PhoenixArrow), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::StormArrow(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherStormArrow);

			if (_ev.m_repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(client->GetMatchId(), SKILL_TYPE::ARCHER_STROM_ARROW, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillInfo->m_posOffset * 2.0f, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 1.f,  skillInfo->m_posOffset * 4.0f }, client->GetWorldMatrix());

				GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);

				_timerQueue.push({ _ev.m_objID, TimeUtil::PassedTimeMSec(skillInfo->m_damageCycleTime), EPlayerSkill::ArcherStormArrow, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {} });
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(client->GetMatchId(), SKILL_TYPE::ARCHER_STROM_ARROW);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer StormArrow), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::MultipleShot(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			CGameMgr::GetInstance()->ArcherMultipleShot(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer MultipleShot), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::ArrowRain(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherArrowRain);

			if (_ev.m_repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::ARCHER_ARROW_RAIN, _ev.m_pos, vec3(0.0f, 0.0f, 0.0f));
				}
			}

			//Boss Collide Check
			if (CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3] != -1) {
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3]);

				CStat stat = client->GetStatus()->GetStat();
				if (DistanceXZ(_ev.m_pos, boss->GetPos()) < skillInfo->m_skillRadius) {
					boss->Damage(_ev.m_power, stat.m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(_ev.m_pos, npc->GetPos()) < skillInfo->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
				}
			}

			if (_ev.m_repeatTime < skillInfo->m_repeatTime) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillInfo->m_damageCycleTime), EPlayerSkill::ArcherArrowRain, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(0, SKILL_TYPE::ARCHER_ARROW_RAIN);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer ArrowRain), " + std::string(ex.what()));
		}

	}
}