#include "pch.h"
#include "CFigtherTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CFigtherTimer::Dodge(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterDodge);

			vec3 newPos = client->GetPos() + ev.pos * (skillCsv->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * TimeUtil::CalElapsedTime(ev.lastProcessTime);

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
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterDodge, ev.pos, 0, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Dodge), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::SpinKick(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterSpinKick);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
			if (bossID != -1) {
				if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
					CObjectMgr::GetInstance()->GetClient(bossID)->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->active)
					continue;
				if (npc->GetBoundingBox().Intersects(box)) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer SpinKick), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::WildAttack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterWildAttack);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);

			if (ev.repeatTime < skillCsv->repeatTime)
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::FighterWildAttack, {}, ev.power, ev.repeatTime + 1, {}));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer WildAttack), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::WindKick(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterWindKick);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);

			vec3 newPos = client->GetPos() + ev.pos * (skillCsv->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * TimeUtil::CalElapsedTime(ev.lastProcessTime);

			float height;
			int curNode;
			if (GameUtil::MapCollision(newPos, height, curNode)) {
				client->SetPos(newPos);
				client->SetCurNode(curNode);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}
			}

			if (ev.repeatTime < skillCsv->repeatTime)
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterWindKick, ev.pos, ev.power, ev.repeatTime + 1, TimeUtil::CurTime()));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer WindKick), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::RisingDragon(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterRisingDragon);

			if (IsFloatEqual(ev.pos.x, 0.f)) {
				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);
					if (DistanceXZ(boss->GetPos(), client->GetPos()) < skillCsv->skillRadius) {
						if (DistanceXZ(boss->GetPos(), client->GetPos()) < skillCsv->extraParam1) {
							boss->Damage(ev.power * 2, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						}
						else {
							boss->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						}
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (DistanceXZ(npc->GetPos(), client->GetPos()) < skillCsv->skillRadius) {
						if (DistanceXZ(npc->GetPos(), client->GetPos()) < skillCsv->extraParam1) {
							npc->Damaged(ev.objID, ev.power * 2, DAMAGE_TYPE::STRENGTH);
						}
						else {
							npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);
						}
					}
				}

			}
			else {
				vec3 newMove = ev.pos;
				vec3 newPos = client->GetPos() + vec3(0.f, ev.pos.x, 0.f) * skillCsv->speed * TimeUtil::CalElapsedTime(ev.lastProcessTime);
				client->SetPos(newPos);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}

				//Check client going up or down
				if (IsFloatEqual(ev.pos.x, 1.f)) {
					if (newPos.y > ev.pos.y) {
						newMove.x = -1.f;
					}
				}
				else {
					float height;
					int curNode;
					GameUtil::MapCollision(client->GetPos(), height, curNode, true);

					if (newPos.y < height) {
						newMove.x = 0.f;
						client->SetPos(client->GetPos().x, height, client->GetPos().z);
						for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
							if (id == -1)
								continue;
							CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
						}
					}
				}
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterRisingDragon, newMove, ev.power, 0, TimeUtil::CurTime()));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer RisingDragon), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::Meditation(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterMeditation);

			if (!client->GetUsingSkill()) {
				CStat stat = client->GetStatus()->GetStat();
				stat.speed += skillCsv->buffInfo[EBuffType::SpeedIncrease].buffValue;
				stat.strength += static_cast<int>(skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
				stat.magic += static_cast<int>(skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
				client->GetStatus()->SetStat(stat);

				CStat changeStat(0);
				changeStat.speed = skillCsv->buffInfo[EBuffType::SpeedIncrease].buffValue;
				changeStat.strength = static_cast<int>(skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);
				changeStat.magic = static_cast<int>(skillCsv->buffInfo[EBuffType::AttackIncrease].buffValue);

				network::GetInstance()->RegisterTimerEvent({ ev.objID, TimeUtil::PassedTimeMSec(skillCsv->buffInfo[EBuffType::SpeedIncrease].buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
			}
			else
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterMeditation, {}, 0, 0, {}));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Meditation), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::DrangonFist(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCSv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterDragonFist);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCSv->posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 1.f, skillCSv->posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);

			if (client->GetUsingSkill()) {
				vec3 newPos = client->GetPos() + ev.pos * (skillCSv->speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().speed)) * TimeUtil::CalElapsedTime(ev.lastProcessTime);

				float height;
				int curNode;
				if (GameUtil::MapCollision(newPos, height, curNode)) {
					client->SetPos(newPos);
					client->SetCurNode(curNode);

					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
					}
				}
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterDragonFist, ev.pos, ev.power, 0, TimeUtil::CurTime()));
			}
			else {
				float height;
				int curNode;
				if (GameUtil::MapCollision(client->GetPos(), height, curNode, true)) {
					client->SetPos(client->GetPos().x, height, client->GetPos().z);
					client->SetCurNode(curNode);

					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
					}
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer DrangonFist), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::Indestructible(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);

			if (client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::INDESTRUCTIBLE) {
				client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;
				client->SetUsingSkill(false);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Indestructible), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::Counter(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterCounter);

			if (client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::COUNTER) {
				client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;

				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID == -1)
					return;
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);

				if (DistanceXZ(client->GetPos(), boss->GetPos()) < skillCsv->skillRadius) {
					boss->Damage(client->GetAccumulatedDamage(), client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, client->GetID());
				}
				else {
					float distance = FLT_MAX;
					int target = -1;
					for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
						std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
						if (!npc->active)
							continue;

						if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->skillRadius) {
							distance = DistanceXZ(client->GetPos(), npc->GetPos());
							target = i;
						}
					}
					if (-1 != target) {
						CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), target)->Damaged(client->GetID(), static_cast<int>(client->GetAccumulatedDamage() * skillCsv->strengthRatio), DAMAGE_TYPE::STRENGTH);
					}
				}

				client->ResetAccumulatedDamage();
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Counter), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::FireBall(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->FireBall(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::FIGHTER_FIREBALL, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer FireBall), " + std::string(ex.what()));
		}
	}
}