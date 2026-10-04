#include "pch.h"
#include "CSwordManTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {

	void CSwordManTimer::Dodge(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			//pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManDodge);

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
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::SwordManDodge, ev.pos, 0, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer Dodge), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::HeavySlash(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManHeavySlash);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID, false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer HeavySlash), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::AuraBlade(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->AuraBlade(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_AURA_BLADE, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer AuraBlade), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::HellBlade(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManHellBlade);
			int matchNum = client->GetMatchNum();

			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)[3];
			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), client->GetPos()) <= skillCsv->skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), client->GetPos()) <= skillCsv->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);
				}
			}

			if (ev.repeatTime < skillCsv->repeatTime) {
				client->SetUsingSkill(true);
				timerQueue.push(SKILL_EVENT(client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::SwordManHellBlade, {}, ev.power, ev.repeatTime + 1, {}));
			}
			else {
				client->SetUsingSkill(false);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer HellBlade), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::JudgementSword(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManJudgementSword);
			if (ev.repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);

				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

				int id = -2;
				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
						CObjectMgr::GetInstance()->GetClient(bossID)->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						id = bossID;
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(client->GetID(), ev.power, DAMAGE_TYPE::STRENGTH);

						if (id != bossID) {
							if (-2 == id) {
								id = i;
							}
							else if (DistanceXZ(client->GetPos(), npc->GetPos()) < DistanceXZ(client->GetPos(), CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), id)->GetPos())) {
								id = i;
							}
						}
					}
				}

				//Sword Damage From Sky
				if (bossID == id) {
					timerQueue.push(SKILL_EVENT(bossID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->extraParam2)), EPlayerSkill::SwordManJudgementSword, vec3(static_cast<float>(client->GetMatchNum()), 0.f, 0.f), ev.power + static_cast<int>(client->GetStatus()->GetStat().magic * 1.2f), 1, {}, client->GetID()));
				}
				else if (-2 != id) {
					timerQueue.push(SKILL_EVENT(CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), id)->GetID(), TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->extraParam2)), EPlayerSkill::SwordManJudgementSword, vec3(static_cast<float>(client->GetMatchNum()), 0.f, 0.f), ev.power + static_cast<int>(client->GetStatus()->GetStat().magic * 1.2f), 1, {}, client->GetID()));
				}
			}
			else if (ev.repeatTime == 1) {
				//pos.x = matchNum
				int objectID;
				vec3 objectPos;
				if (ev.objID >= NPC_ID) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(ev.pos.x), ev.objID - NPC_ID);
					objectPos = npc->GetPos() + vec3(0.f, skillCsv->posOffset, 0.f);
					objectID = CGameMgr::GetInstance()->JudgeMentSword(static_cast<int>(ev.pos.x), objectPos, ev.power, 0, ev.objID, ev.clientID);
				}
				else {
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
					objectPos = client->GetPos() + vec3(0.f, skillCsv->posOffset, 0.f);
					objectID = CGameMgr::GetInstance()->JudgeMentSword(static_cast<int>(ev.pos.x), objectPos, ev.power, client->GetStatus()->GetStat().critical, ev.objID, ev.clientID);
				}

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(static_cast<int>(ev.pos.x))) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD, objectPos, vec3(0.f, 0.f, 1.f));
				}
			}
			else {
				//power = matchNum
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(ev.power)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(ev.objID, static_cast<SKILL_TYPE>(ev.clientID));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer JudgementSword), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::AnkleCut(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManAnkleCut);
			if (ev.repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				auto slowDebuff = skillCsv->debuffInfo[EDebuffType::Slow];

				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());

				CStat changeStat = CStat(0);
				changeStat.speed = -slowDebuff.debuffValue;
				

				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
						CObjectMgr::GetInstance()->GetClient(bossID)->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::STRENGTH, ev.objID);
						CStat stat = CObjectMgr::GetInstance()->GetClient(bossID)->GetStatus()->GetStat();
						stat.speed -= slowDebuff.debuffValue;
						CObjectMgr::GetInstance()->GetClient(bossID)->GetStatus()->SetStat(stat);
						timerQueue.push(SKILL_EVENT(bossID, TimeUtil::CurTime(), EPlayerSkill::SwordManAnkleCut, {}, client->GetStatus()->GetStat().strength + client->GetStatus()->GetStat().magic, 1, {}));
						network::GetInstance()->RegisterTimerEvent({ bossID, TimeUtil::PassedTimeMSec(slowDebuff.debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::STRENGTH);

						float speed = npc->GetSpeed();
						speed -= slowDebuff.debuffValue;
						npc->SetSpeed(speed);

						timerQueue.push(SKILL_EVENT(npc->GetID(), TimeUtil::CurTime(), EPlayerSkill::SwordManAnkleCut, vec3(static_cast<float>(npc->GetMatchNum()), 0.f, 0.f), client->GetStatus()->GetStat().strength + client->GetStatus()->GetStat().magic, 1, {}, ev.objID));
						network::GetInstance()->RegisterTimerEvent({ npc->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, npc->GetMatchNum(), changeStat });
					}
				}
			}
			else {
				auto bleedDebuff = skillCsv->debuffInfo[EDebuffType::Bleed];
				if (ev.objID >= NPC_ID) {
					//ev.pos.x = matchNum
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(ev.pos.x), ev.objID - NPC_ID);
					npc->Damaged(ev.clientID, ev.power, DAMAGE_TYPE::STRENGTH, false);
					if (ev.repeatTime < bleedDebuff.debuffRepeatTime)
						timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(bleedDebuff.debuffDuration), EPlayerSkill::SwordManAnkleCut, ev.pos, ev.power, ev.repeatTime + 1, {}));
				}
				else {
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
					client->Damage(ev.power, 0, DAMAGE_TYPE::STRENGTH, ev.objID);
					//Damage client
					if (ev.repeatTime < bleedDebuff.debuffRepeatTime)
						timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(bleedDebuff.debuffDuration), EPlayerSkill::SwordManAnkleCut, {}, ev.power, ev.repeatTime + 1, {}));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer AnkleCut), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::ShieldBash(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			//pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManShieldBash);

			//Check Collision
			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos(), client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset }, client->GetWorldMatrix());

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

			// Move Client
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
				timerQueue.push({ ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::SwordManShieldBash, ev.pos, ev.power, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer ShieldBash), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::ProtectedArea(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			if (ev.repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManProtectedArea);
				int objectID = CGameMgr::GetInstance()->ProtectedArea(client->GetMatchNum(), ev.pos, ev.power, ev.objID);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_PROTECTED_AREA, ev.pos, client->GetLook());
				}
				timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->extraParam2)), EPlayerSkill::SwordManProtectedArea, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchID
				CGameMgr::GetInstance()->ProtectedArea(ev.power, ev.objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer ProtectedArea), " + std::string(ex.what()));
		}
	}
}