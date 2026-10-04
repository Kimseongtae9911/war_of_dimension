#include "pch.h"
#include "CProTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CProTimer::Attack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->ProAttack(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::PRO_ATTACK, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Attack), " + std::string(ex.what()));
		}
	}

	void CProTimer::Pointer(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			std::shared_ptr<CClient> target = CObjectMgr::GetInstance()->GetClient(ev.clientID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPointer);

			vec3 newPos = target->GetPos() - target->GetLook() * skillCsv->posOffset;
			client->SetPos(newPos);
			client->SetLook(vec3::Normalize(target->GetPos() - client->GetPos()));

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), newPos, 0);
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRotatePacket(client->GetMatchId(), client->GetLook(), client->GetRight());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Pointer), " + std::string(ex.what()));
		}
	}

	void CProTimer::Release(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerRelease);
			auto debuff = skillCsv->debuffInfo[EDebuffType::MemoryLeak];

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum);
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 != clientIDs[i]) {
					if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos(), ev.pos) <= skillCsv->skillRadius) {
						CObjectMgr::GetInstance()->GetClient(clientIDs[i])->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID);
						timerQueue.push(SKILL_EVENT(clientIDs[i], TimeUtil::CurTime(), EPlayerSkill::MemoryLeak, vec3(static_cast<float>(matchNum), 0.f, 0.f), static_cast<int>(ev.power * debuff.debuffValue), 1, {}, ev.objID));
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
					timerQueue.push(SKILL_EVENT(npc->GetID(), TimeUtil::CurTime(), EPlayerSkill::MemoryLeak, vec3(static_cast<float>(matchNum), 0.f, 0.f), static_cast<int>(ev.power * debuff.debuffValue), 1, {}, ev.objID));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Release), " + std::string(ex.what()));
		}
	}

	void CProTimer::Delete(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerDelete);
			auto debuff = skillCsv->debuffInfo[EDebuffType::Silence];

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum);
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 != clientIDs[i]) {
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(hero->GetPos(), ev.pos) <= skillCsv->skillRadius) {
						hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID);
						hero->GetStatus()->skillBuff = SKILL_BUFF::SILENCE;

						for (int j = 0; j < MAX_PLAYER; ++j) {
							if (-1 == clientIDs[j])
								continue;
							CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender()->SendPlayerStatusChangePakcet(hero->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::SILENCE));
						}
						timerQueue.push(SKILL_EVENT(hero->GetID(), TimeUtil::PassedTimeMSec(debuff.debuffDuration), EPlayerSkill::Silence, {}, 0, 0, {}));
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Delete), " + std::string(ex.what()));
		}
	}

	void CProTimer::SCL(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerLaser);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());
			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID, false);

			if (ev.repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::PRO_SCL, client->GetPos() + client->GetLook() * 2.f + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::ProgrammerLaser, ev.pos, ev.power, ev.repeatTime + 1, {}));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(0, SKILL_TYPE::PRO_SCL);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer SCL), " + std::string(ex.what()));
		}
	}

	void CProTimer::WhileTrue(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerWhileTrue);

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());

			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (clientIDs[i] == -1)
					continue;
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
				if (DistanceXZ(hero->GetPos(), client->GetPos()) < skillCsv->skillRadius) {
					hero->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID);
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), client->GetPos()) <= skillCsv->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
				}
			}

			//Mp Consuption
			client->GetStatus()->healthMana.UseMp(static_cast<int>(skillCsv->extraParam1) + ev.repeatTime * static_cast<int>(skillCsv->extraParam2));
			if (client->GetStatus()->healthMana.GetCurMp() < skillCsv->extraParam1 + (ev.repeatTime + 1) * skillCsv->extraParam2) {
				CGameMgr::GetInstance()->WhileTrue(false, client->GetMatchNum());
			}

			for (int id : clientIDs) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->healthMana);
			}

			if (CGameMgr::GetInstance()->GetWhileTrue(client->GetMatchNum())) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::ProgrammerWhileTrue, {}, ev.power, ev.repeatTime + 1, TimeUtil::CurTime()));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer WhileTrue), " + std::string(ex.what()));
		}
	}

	void CProTimer::HelloWorld(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			if (ev.repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerHelloWorld);

				std::vector<int> area;
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (-1 == clientIDs[i])
						continue;
					if (DistanceXZ(client->GetPos(), CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos()) < skillCsv->skillRadius) {
						area.push_back(clientIDs[i]);
					}
				}


				int objectID = CGameMgr::GetInstance()->HelloWorld(client->GetMatchNum(), ev.pos, ev.objID, area);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::PRO_HELLO_WORLD, ev.pos, client->GetLook());
				}
				timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->extraParam1)), EPlayerSkill::ProgrammerHelloWorld, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchID
				CGameMgr::GetInstance()->HelloWorld(ev.power, ev.objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer HelloWorld), " + std::string(ex.what()));
		}
	}
}