#include "pch.h"
#include "CFigtherTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CFigtherTimer::Dodge(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterDodge);

			vec3 newPos = client->GetPos() + _ev.m_pos * (skillCsv->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

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
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterDodge, _ev.m_pos, 0, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Dodge), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::SpinKick(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterSpinKick);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->m_posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 3.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
			if (bossID != -1) {
				if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
					CObjectMgr::GetInstance()->GetClient(bossID)->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->m_active)
					continue;
				if (npc->GetBoundingBox().Intersects(box)) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer SpinKick), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::WildAttack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterWildAttack);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->m_posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);

			if (_ev.m_repeatTime < skillCsv->m_repeatTime)
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::FighterWildAttack, {}, _ev.m_power, _ev.m_repeatTime + 1, {}));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer WildAttack), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::WindKick(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterWindKick);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCsv->m_posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);

			vec3 newPos = client->GetPos() + _ev.m_pos * (skillCsv->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

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

			if (_ev.m_repeatTime < skillCsv->m_repeatTime)
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterWindKick, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, TimeUtil::CurTime()));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer WindKick), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::RisingDragon(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterRisingDragon);

			if (IsFloatEqual(_ev.m_pos.m_x, 0.f)) {
				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);
					if (DistanceXZ(boss->GetPos(), client->GetPos()) < skillCsv->m_skillRadius) {
						if (DistanceXZ(boss->GetPos(), client->GetPos()) < skillCsv->m_extraParam1) {
							boss->Damage(_ev.m_power * 2, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						}
						else {
							boss->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						}
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (DistanceXZ(npc->GetPos(), client->GetPos()) < skillCsv->m_skillRadius) {
						if (DistanceXZ(npc->GetPos(), client->GetPos()) < skillCsv->m_extraParam1) {
							npc->Damaged(_ev.m_objID, _ev.m_power * 2, DAMAGE_TYPE::STRENGTH);
						}
						else {
							npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);
						}
					}
				}

			}
			else {
				vec3 newMove = _ev.m_pos;
				vec3 newPos = client->GetPos() + vec3(0.f, _ev.m_pos.m_x, 0.f) * skillCsv->m_speed * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);
				client->SetPos(newPos);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}

				//Check client going up or down
				if (IsFloatEqual(_ev.m_pos.m_x, 1.f)) {
					if (newPos.m_y > _ev.m_pos.m_y) {
						newMove.m_x = -1.f;
					}
				}
				else {
					float height;
					int curNode;
					GameUtil::MapCollision(client->GetPos(), height, curNode, true);

					if (newPos.m_y < height) {
						newMove.m_x = 0.f;
						client->SetPos(client->GetPos().m_x, height, client->GetPos().m_z);
						for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
							if (id == -1)
								continue;
							CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
						}
					}
				}
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterRisingDragon, newMove, _ev.m_power, 0, TimeUtil::CurTime()));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer RisingDragon), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::Meditation(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterMeditation);

			if (!client->GetUsingSkill()) {
				CStat stat = client->GetStatus()->GetStat();
				stat.m_speed += skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffValue;
				stat.m_strength += static_cast<int>(skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
				stat.m_magic += static_cast<int>(skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
				client->GetStatus()->SetStat(stat);

				CStat changeStat(0);
				changeStat.m_speed = skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffValue;
				changeStat.m_strength = static_cast<int>(skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);
				changeStat.m_magic = static_cast<int>(skillCsv->m_buffInfo[EBuffType::AttackIncrease].m_buffValue);

				network::GetInstance()->RegisterTimerEvent({ _ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_buffInfo[EBuffType::SpeedIncrease].m_buffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat});
			}
			else
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterMeditation, {}, 0, 0, {}));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Meditation), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::DrangonFist(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			//ev.pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCSv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterDragonFist);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos() + client->GetLook() * skillCSv->m_posOffset, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 1.f, skillCSv->m_posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);

			if (client->GetUsingSkill()) {
				vec3 newPos = client->GetPos() + _ev.m_pos * (skillCSv->m_speed + SKILL_ADDITIONAL_SPEED_FROM_STAT(client->GetStatus()->GetStat().m_speed)) * TimeUtil::CalElapsedTime(_ev.m_lastProcessTime);

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
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::FighterDragonFist, _ev.m_pos, _ev.m_power, 0, TimeUtil::CurTime()));
			}
			else {
				float height;
				int curNode;
				if (GameUtil::MapCollision(client->GetPos(), height, curNode, true)) {
					client->SetPos(client->GetPos().m_x, height, client->GetPos().m_z);
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

	void CFigtherTimer::Indestructible(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);

			if (client->GetStatus()->m_defensiveBuff == DEFENSIVE_BUFF::INDESTRUCTIBLE) {
				client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;
				client->SetUsingSkill(false);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Indestructible), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::Counter(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::FighterCounter);

			if (client->GetStatus()->m_defensiveBuff == DEFENSIVE_BUFF::COUNTER) {
				client->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;

				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID == -1)
					return;
				std::shared_ptr<CClient> boss = CObjectMgr::GetInstance()->GetClient(bossID);

				if (DistanceXZ(client->GetPos(), boss->GetPos()) < skillCsv->m_skillRadius) {
					boss->Damage(client->GetAccumulatedDamage(), client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, client->GetID());
				}
				else {
					float distance = FLT_MAX;
					int target = -1;
					for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
						std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
						if (!npc->m_active)
							continue;

						if (DistanceXZ(client->GetPos(), npc->GetPos()) < skillCsv->m_skillRadius) {
							distance = DistanceXZ(client->GetPos(), npc->GetPos());
							target = i;
						}
					}
					if (-1 != target) {
						CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), target)->Damaged(client->GetID(), static_cast<int>(client->GetAccumulatedDamage() * skillCsv->m_strengthRatio), DAMAGE_TYPE::STRENGTH);
					}
				}

				client->ResetAccumulatedDamage();
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer Counter), " + std::string(ex.what()));
		}
	}

	void CFigtherTimer::FireBall(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->FireBall(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);
			if (objectID == -1)
				return;
			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::FIGHTER_FIREBALL, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CFigtherTimer FireBall), " + std::string(ex.what()));
		}
	}
}