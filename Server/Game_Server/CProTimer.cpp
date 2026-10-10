#include "pch.h"
#include "CProTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {
	void CProTimer::Attack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->ProAttack(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::PRO_ATTACK, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Attack), " + std::string(ex.what()));
		}
	}

	void CProTimer::Pointer(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			std::shared_ptr<CClient> target = CObjectMgr::GetInstance()->GetClient(_ev.m_clientID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerPointer);

			vec3 newPos = target->GetPos() - target->GetLook() * skillCsv->m_posOffset;
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

	void CProTimer::Release(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerRelease);
			auto debuff = skillCsv->m_debuffInfo[EDebuffType::MemoryLeak];

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum);
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 != clientIDs[i]) {
					if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
						CObjectMgr::GetInstance()->GetClient(clientIDs[i])->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID);
						_timerQueue.push(SKILL_EVENT(clientIDs[i], TimeUtil::CurTime(), EPlayerSkill::MemoryLeak, vec3(static_cast<float>(matchNum), 0.f, 0.f), static_cast<int>(_ev.m_power * debuff.m_debuffValue), 1, {}, _ev.m_objID));
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
					_timerQueue.push(SKILL_EVENT(npc->GetID(), TimeUtil::CurTime(), EPlayerSkill::MemoryLeak, vec3(static_cast<float>(matchNum), 0.f, 0.f), static_cast<int>(_ev.m_power * debuff.m_debuffValue), 1, {}, _ev.m_objID));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Release), " + std::string(ex.what()));
		}
	}

	void CProTimer::Delete(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerDelete);
			auto debuff = skillCsv->m_debuffInfo[EDebuffType::Silence];

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum);
			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (-1 != clientIDs[i]) {
					std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
					if (DistanceXZ(hero->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
						hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID);
						hero->GetStatus()->m_skillBuff = SKILL_BUFF::SILENCE;

						for (int j = 0; j < MAX_PLAYER; ++j) {
							if (-1 == clientIDs[j])
								continue;
							CObjectMgr::GetInstance()->GetClient(clientIDs[j])->GetPacketSender()->SendPlayerStatusChangePakcet(hero->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::SILENCE));
						}
						_timerQueue.push(SKILL_EVENT(hero->GetID(), TimeUtil::PassedTimeMSec(debuff.m_debuffDuration), EPlayerSkill::Silence, {}, 0, 0, {}));
					}
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer Delete), " + std::string(ex.what()));
		}
	}

	void CProTimer::SCL(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerLaser);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());
			GameUtil::BossSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID, false);

			if (_ev.m_repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::PRO_SCL, client->GetPos() + client->GetLook() * 2.f + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::ProgrammerLaser, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}));
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

	void CProTimer::WhileTrue(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerWhileTrue);

			std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());

			for (int i = 0; i < MAX_PLAYER - 1; ++i) {
				if (clientIDs[i] == -1)
					continue;
				std::shared_ptr<CClient> hero = CObjectMgr::GetInstance()->GetClient(clientIDs[i]);
				if (DistanceXZ(hero->GetPos(), client->GetPos()) < skillCsv->m_skillRadius) {
					hero->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID);
				}
			}

			for (int i = MAX_MINION; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), client->GetPos()) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
				}
			}

			//Mp Consuption
			client->GetStatus()->m_healthMana.UseMp(static_cast<int>(skillCsv->m_extraParam1) + _ev.m_repeatTime * static_cast<int>(skillCsv->m_extraParam2));
			if (client->GetStatus()->m_healthMana.GetCurMp() < skillCsv->m_extraParam1 + (_ev.m_repeatTime + 1) * skillCsv->m_extraParam2) {
				CGameMgr::GetInstance()->WhileTrue(false, client->GetMatchNum());
			}

			for (int id : clientIDs) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerHealthManaPacket(client->GetMatchId(), client->GetStatus()->m_healthMana);
			}

			if (CGameMgr::GetInstance()->GetWhileTrue(client->GetMatchNum())) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::ProgrammerWhileTrue, {}, _ev.m_power, _ev.m_repeatTime + 1, TimeUtil::CurTime()));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer WhileTrue), " + std::string(ex.what()));
		}
	}

	void CProTimer::HelloWorld(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			if (_ev.m_repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::ProgrammerHelloWorld);

				std::vector<int> area;
				std::array<int, MAX_PLAYER> clientIDs = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum());
				for (int i = 0; i < MAX_PLAYER - 1; ++i) {
					if (-1 == clientIDs[i])
						continue;
					if (DistanceXZ(client->GetPos(), CObjectMgr::GetInstance()->GetClient(clientIDs[i])->GetPos()) < skillCsv->m_skillRadius) {
						area.push_back(clientIDs[i]);
					}
				}


				int objectID = CGameMgr::GetInstance()->HelloWorld(client->GetMatchNum(), _ev.m_pos, _ev.m_objID, area);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::PRO_HELLO_WORLD, _ev.m_pos, client->GetLook());
				}
				_timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->m_extraParam1)), EPlayerSkill::ProgrammerHelloWorld, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchID
				CGameMgr::GetInstance()->HelloWorld(_ev.m_power, _ev.m_objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CProTimer HelloWorld), " + std::string(ex.what()));
		}
	}
}