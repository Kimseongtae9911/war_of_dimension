#include "pch.h"
#include "CArcherTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CArcherTimer::Attack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherAttack);
		
		const auto stat = client->GetStatus()->GetStat();
		const auto objectPos = skillInfo->GetStartPos(client->GetPos(), client->GetLook()) + ClientInfos::HERO_ATTACK_OFFSET;
		const auto damage = skillInfo->GetDamage(stat.strength, stat.magic);
		int objectID = CGameMgr::GetInstance()->ArcherAttack(client->GetMatchNum(), objectPos, client->GetLook(), damage, stat.critical, ev.objID);
		if (objectID == -1)
			return;
		for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
			if (-1 == id)
				continue;
			CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_ATTACK, objectPos, client->GetLook());
		}
	}

	void CArcherTimer::Vault(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherVault);

		const auto stat = client->GetStatus()->GetStat();
		const auto objectPos = skillInfo->GetStartPos(client->GetPos(), client->GetLook());
		const auto damage = skillInfo->GetDamage(stat.strength, stat.magic);
		if (ev.repeatTime == 0) {

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(objectPos, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 1.f, 3.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), damage, stat.critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);

			if (client->GetUsingSkill())
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherVault, client->GetLook(), 0, ev.repeatTime + 1, TimeUtil::CurTime() });
		}
		else {
			//ev.pos = look
			vec3 newPos = client->GetPos() - ev.pos * skillInfo->speed * TimeUtil::CalElapsedTime(ev.lastProcessTime);

			float height;
			int curNode;
			if (GameUtil::MapCollision(newPos, height, curNode)) {
				newPos.y = height;
				client->SetPos(newPos);
				client->SetCurNode(curNode);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}
			}

			if (ev.repeatTime < skillInfo->repeatTime)
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherVault, ev.pos, ev.power, ev.repeatTime + 1, TimeUtil::CurTime() });
		}
	}
	
	void CArcherTimer::BackStep(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherBackStep);

		vec3 newPos = client->GetPos() + ev.pos * (skillInfo->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * TimeUtil::CalElapsedTime(ev.lastProcessTime);

		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
			}
		}

		if (client->GetUsingSkill())
			timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherBackStep, ev.pos, 0, 0, TimeUtil::CurTime() });
	}

	void CArcherTimer::Dodge(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
		const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherDodge);		

		vec3 newPos = client->GetPos() + ev.pos * (skillInfo->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * TimeUtil::CalElapsedTime(ev.lastProcessTime);

		float height;
		int curNode;
		if (GameUtil::MapCollision(newPos, height, curNode)) {
			newPos.y = height;
			client->SetPos(newPos);
			client->SetCurNode(curNode);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
			}
		}

		if (client->GetUsingSkill())
			timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::ArcherDodge, ev.pos, 0, 0, TimeUtil::CurTime() });
	}

	void CArcherTimer::PenetraitingShot(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->PenetraitingShot(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_PENETRAITING_SHOT, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer PenetraitingShot), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::StickyArrow(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->StickyArrow(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_STICKY_ARROW, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer StickyArrow), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::PhoenixArrow(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->PhoenixArrow(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::ARCHER_PHOENIX_ARROW, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer PhoenixArrow), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::StormArrow(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherStormArrow);

			if (ev.repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(client->GetMatchId(), SKILL_TYPE::ARCHER_STROM_ARROW, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillInfo->posOffset * 2.0f, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 1.f,  skillInfo->posOffset * 4.0f }, client->GetWorldMatrix());

				GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);

				timerQueue.push({ ev.objID, TimeUtil::PassedTimeMSec(skillInfo->damageCycleTime), EPlayerSkill::ArcherStormArrow, ev.pos, ev.power, ev.repeatTime + 1, {} });
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

	void CArcherTimer::MultipleShot(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			CGameMgr::GetInstance()->ArcherMultipleShot(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CArcherTimer MultipleShot), " + std::string(ex.what()));
		}
	}

	void CArcherTimer::ArrowRain(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			const auto skillInfo = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ArcherArrowRain);

			if (ev.repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::ARCHER_ARROW_RAIN, ev.pos, vec3(0.0f, 0.0f, 0.0f));
				}
			}

			//Boss Collide Check
			if (CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3] != -1) {
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3]);

				CStat stat = client->GetStatus()->GetStat();
				if (DistanceXZ(ev.pos, boss->GetPos()) < skillInfo->skillRadius) {
					boss->Damage(ev.power, stat.critical, DAMAGE_TYPE::MAGIC, ev.objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->active)
					continue;
				if (DistanceXZ(ev.pos, npc->GetPos()) < skillInfo->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
				}
			}

			if (ev.repeatTime < skillInfo->repeatTime) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillInfo->damageCycleTime), EPlayerSkill::ArcherArrowRain, ev.pos, ev.power, ev.repeatTime + 1, {}));
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