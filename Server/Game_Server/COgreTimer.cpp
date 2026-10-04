#include "pch.h"
#include "COgreTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {

	void COgreTimer::Attack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreAttack);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 1.5f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			//Tower Collide Check
			for (int i = 0; i < PATH_NUM; ++i) {
				if (CGameMgr::GetInstance()->GetTower(matchNum, i)->GetBroken() || !CGameMgr::GetInstance()->GetTower(matchNum, i)->active)
					continue;
				if (GameUtil::GetTowerBB(i).Intersects(box)) {
					CGameMgr::GetInstance()->GetTower(matchNum, i)->Damage(ev.power);
					return;
				}
			}

			//Nexus Collide Check
			if (GameUtil::GetNexusBB().Intersects(box)) {
				CGameMgr::GetInstance()->GetNexus(matchNum)->Damage(ev.power);
				return;
			}

			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, client->GetID(), false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Attack), " + std::string(ex.what()));
		}
	}

	void COgreTimer::HeavySwing(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreHeavySwing);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 4.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, client->GetID(), false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer HeavySwing), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Crunch(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreCrunch);
			auto slowDebuff = skillCsv->debuffInfo[EDebuffType::Slow];

			vec3 newPos = client->GetPos() + ev.pos * skillCsv->speed * TimeUtil::CalElapsedTime(ev.lastProcessTime);

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

			if (ev.repeatTime > skillCsv->repeatTime) {
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (clientIDs[i] == -1)
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(client->GetPos(), hero->GetPos()) < skillCsv->skillRadius) {
						hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						CStat heroStat = hero->GetStatus()->GetStat();
						heroStat.speed -= slowDebuff.debuffValue;
						hero->GetStatus()->SetStat(heroStat);

						CStat changeStat(0);
						changeStat.speed = -slowDebuff.debuffValue;
						network::GetInstance()->RegisterTimerEvent({ clientIDs[i], TimeUtil::PassedTimeMSec(slowDebuff.debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
					}
					CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}

				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->skillRadius) {
						npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);
					}
				}
			}
			else {
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::OgreCrunch, ev.pos, ev.power, ev.repeatTime + 1,TimeUtil::CurTime() });
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Crunch), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Endure(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreEndure);
			auto buff = skillCsv->buffInfo[EBuffType::AttackIncrease];

			if (client->GetStatus()->defensiveBuff == DEFENSIVE_BUFF::ENDURE) {
				client->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;
				//Attack Increase
				CStat stat = client->GetStatus()->GetStat();
				stat.strength += static_cast<int>(client->GetAccumulatedDamage() * buff.buffValue);
				client->GetStatus()->SetStat(stat);

				CStat changeStat(0);
				changeStat.strength = static_cast<int>(client->GetAccumulatedDamage() * buff.buffValue);
				network::GetInstance()->RegisterTimerEvent({ ev.objID, TimeUtil::PassedTimeMSec(buff.buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
				client->ResetAccumulatedDamage();
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Endure), " + std::string(ex.what()));
		}
	}

	void COgreTimer::RockThrow(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->RockThrow(client->GetMatchNum(), ev.pos + ClientInfos::OGRE_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::OGRE_ROCK_THROW, ev.pos + ClientInfos::OGRE_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer RockThrow), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Butting(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreButting);
			auto stunDebuff = skillCsv->debuffInfo[EDebuffType::Stun];

			if (ev.repeatTime == 0) {
				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 1.5f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

				//Hero Collide Check
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (-1 == clientIDs[i])
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (hero->GetBoundingBox().Intersects(box)) {
						hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						hero->GetStatus()->skillBuff = SKILL_BUFF::STUN;

						for (int j = 0; j < MAX_PLAYER; ++j) {
							if (-1 == clientIDs[j])
								continue;
							CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender()->SendPlayerStatusChangePakcet(hero->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::STUN));
						}

						timerQueue.push(SKILL_EVENT(hero->GetID(), TimeUtil::PassedTimeMSec(stunDebuff.debuffDuration), EPlayerSkill::OgreButting, {}, 0, 1, {}));
						break;
					}
				}

				//Monster Collide Check
				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(client->GetID(), ev.power, DAMAGE_TYPE::STRENGTH);
						break;
					}
				}
			}
			else {
				//Stun Release
				if (client->GetStatus()->skillBuff == SKILL_BUFF::STUN) {
					client->GetStatus()->skillBuff = SKILL_BUFF::NONE;
					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (-1 == id)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::NONE));
					}
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Butting), " + std::string(ex.what()));
		}
	}

	void COgreTimer::DimensionCrush(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			if (ev.repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreDimensionCrush);

				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (clientIDs[i] == -1)
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(client->GetPos(), hero->GetPos()) < skillCsv->skillRadius) {
						hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
					}
				}

				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->skillRadius) {
						npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);
					}
				}

				int objectID = CGameMgr::GetInstance()->DimensionCrush(client->GetMatchNum(), ev.pos, client->GetStatus()->GetStat().critical, static_cast<int>(ev.power * skillCsv->extraParam2 + client->GetStatus()->GetStat().magic * skillCsv->extraParam3), ev.objID);
				for (int id : clientIDs) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::OGRE_DIMENSION_CRUSH, ev.pos, client->GetLook());
				}

				timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(skillCsv->repeatTime), EPlayerSkill::OgreDimensionCrush, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchNum
				CGameMgr::GetInstance()->DimensionCrush(ev.power, ev.objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer DimensionCrush), " + std::string(ex.what()));
		}
	}

	void COgreTimer::DimensionPunch(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreDimensionPunch);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos + ClientInfos::OGRE_ATTACK_OFFSET, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 6.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());


			int objectID = 0;
			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 == clientIDs[i])
					continue;
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
				if (hero->GetBoundingBox().Intersects(box)) {
					hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID);
					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID++, SKILL_TYPE::OGRE_DIMENSION_PUNCH, hero->GetPos() - client->GetLook() * 1.0f, client->GetLook());
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->active)
					continue;
				if (npc->GetBoundingBox().Intersects(box)) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID++, SKILL_TYPE::OGRE_DIMENSION_PUNCH, npc->GetPos() - client->GetLook() * 1.0f, client->GetLook());
					}
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer DimensionPunch), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Gluttony(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);

			if (client->GetStatus()->damageBuff == DAMAGE_BUFF::GLUTTONY)
				client->GetStatus()->damageBuff = DAMAGE_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Gluttony), " + std::string(ex.what()));
		}
	}
}