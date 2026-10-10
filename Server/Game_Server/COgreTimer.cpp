#include "pch.h"
#include "COgreTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {

	void COgreTimer::Attack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreAttack);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 1.5f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			//Tower Collide Check
			for (int i = 0; i < PATH_NUM; ++i) {
				if (CGameMgr::GetInstance()->GetTower(matchNum, i)->GetBroken() || !CGameMgr::GetInstance()->GetTower(matchNum, i)->m_active)
					continue;
				if (GameUtil::GetTowerBB(i).Intersects(box)) {
					CGameMgr::GetInstance()->GetTower(matchNum, i)->Damage(_ev.m_power);
					return;
				}
			}

			//Nexus Collide Check
			if (GameUtil::GetNexusBB().Intersects(box)) {
				CGameMgr::GetInstance()->GetNexus(matchNum)->Damage(_ev.m_power);
				return;
			}

			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, client->GetID(), false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Attack), " + std::string(ex.what()));
		}
	}

	void COgreTimer::HeavySwing(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreHeavySwing);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 4.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, client->GetID(), false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer HeavySwing), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Crunch(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreCrunch);
			auto slowDebuff = skillCsv->m_debuffInfo[EDebuffType::Slow];

			vec3 newPos = client->GetPos() + _ev.m_pos * skillCsv->m_speed * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

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

			if (_ev.m_repeatTime > skillCsv->m_repeatTime) {
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (clientIDs[i] == -1)
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(client->GetPos(), hero->GetPos()) < skillCsv->m_skillRadius) {
						hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						CStat heroStat = hero->GetStatus()->GetStat();
						heroStat.m_speed -= slowDebuff.m_debuffValue;
						hero->GetStatus()->SetStat(heroStat);

						CStat changeStat(0);
						changeStat.m_speed = -slowDebuff.m_debuffValue;
						network::GetInstance()->RegisterTimerEvent({ clientIDs[i], TimeUtil::PassedTimeMSec(slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
					}
					CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}

				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->m_skillRadius) {
						npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);
					}
				}
			}
			else {
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::OgreCrunch, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1,TimeUtil::CurTime() });
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Crunch), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Endure(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreEndure);
			auto buff = skillCsv->m_buffInfo[EBuffType::AttackIncrease];

			if (client->GetStatus()->m_defensiveBuff == DEFENSIVE_BUFF::ENDURE) {
				client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;
				//Attack Increase
				CStat stat = client->GetStatus()->GetStat();
				stat.m_strength += static_cast<int>(client->GetAccumulatedDamage() * buff.m_buffValue);
				client->GetStatus()->SetStat(stat);

				CStat changeStat(0);
				changeStat.m_strength = static_cast<int>(client->GetAccumulatedDamage() * buff.m_buffValue);
				network::GetInstance()->RegisterTimerEvent({ _ev.m_objID, TimeUtil::PassedTimeMSec(buff.m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
				client->ResetAccumulatedDamage();
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Endure), " + std::string(ex.what()));
		}
	}

	void COgreTimer::RockThrow(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->RockThrow(client->GetMatchNum(), _ev.m_pos + ClientInfos::OGRE_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::OGRE_ROCK_THROW, _ev.m_pos + ClientInfos::OGRE_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer RockThrow), " + std::string(ex.what()));
		}
	}

	void COgreTimer::Butting(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreButting);
			auto stunDebuff = skillCsv->m_debuffInfo[EDebuffType::Stun];

			if (_ev.m_repeatTime == 0) {
				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 1.5f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

				//Hero Collide Check
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (-1 == clientIDs[i])
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (hero->GetBoundingBox().Intersects(box)) {
						hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						hero->GetStatus()->m_skillBuff = SKILL_BUFF::STUN;

						for (int j = 0; j < MAX_PLAYER; ++j) {
							if (-1 == clientIDs[j])
								continue;
							CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender()->SendPlayerStatusChangePakcet(hero->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::STUN));
						}

						_timerQueue.push(SKILL_EVENT(hero->GetID(), TimeUtil::PassedTimeMSec(stunDebuff.m_debuffDuration), EPlayerSkill::OgreButting, {}, 0, 1, {}));
						break;
					}
				}

				//Monster Collide Check
				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(client->GetID(), _ev.m_power, DAMAGE_TYPE::STRENGTH);
						break;
					}
				}
			}
			else {
				//Stun Release
				if (client->GetStatus()->m_skillBuff == SKILL_BUFF::STUN) {
					client->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;
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

	void COgreTimer::DimensionCrush(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			if (_ev.m_repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreDimensionCrush);

				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (clientIDs[i] == -1)
						continue;
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(client->GetPos(), hero->GetPos()) < skillCsv->m_skillRadius) {
						hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
					}
				}

				for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->m_skillRadius) {
						npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);
					}
				}

				int objectID = CGameMgr::GetInstance()->DimensionCrush(client->GetMatchNum(), _ev.m_pos, client->GetStatus()->GetStat().m_critical, static_cast<int>(_ev.m_power * skillCsv->m_extraParam2 + client->GetStatus()->GetStat().m_magic * skillCsv->m_extraParam3), _ev.m_objID);
				for (int id : clientIDs) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::OGRE_DIMENSION_CRUSH, _ev.m_pos, client->GetLook());
				}

				_timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(skillCsv->m_repeatTime), EPlayerSkill::OgreDimensionCrush, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchNum
				CGameMgr::GetInstance()->DimensionCrush(_ev.m_power, _ev.m_objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer DimensionCrush), " + std::string(ex.what()));
		}
	}

	void COgreTimer::DimensionPunch(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::OgreDimensionPunch);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos + ClientInfos::OGRE_ATTACK_OFFSET, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 6.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());


			int objectID = 0;
			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 == clientIDs[i])
					continue;
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
				if (hero->GetBoundingBox().Intersects(box)) {
					hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID);
					for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
						if (id == -1)
							continue;
						CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID++, SKILL_TYPE::OGRE_DIMENSION_PUNCH, hero->GetPos() - client->GetLook() * 1.0f, client->GetLook());
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->m_active)
					continue;
				if (npc->GetBoundingBox().Intersects(box)) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
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

	void COgreTimer::Gluttony(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);

			if (client->GetStatus()->m_damageBuff == DAMAGE_BUFF::GLUTTONY)
				client->GetStatus()->m_damageBuff = DAMAGE_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(COgreTimer Gluttony), " + std::string(ex.what()));
		}
	}
}