#include "pch.h"
#include "CSwordManTimer.h"
#include "CClient.h"
#include "ClientInfos.h"

namespace wod_server {

	void CSwordManTimer::Dodge(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			//pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManDodge);

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
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::SwordManDodge, _ev.m_pos, 0, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer Dodge), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::HeavySlash(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManHeavySlash);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID, false);
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer HeavySlash), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::AuraBlade(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->AuraBlade(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (id == -1)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_AURA_BLADE, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer AuraBlade), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::HellBlade(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManHellBlade);
			int matchNum = client->GetMatchNum();

			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)[3];
			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), client->GetPos()) <= skillCsv->m_skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), client->GetPos()) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);
				}
			}

			if (_ev.m_repeatTime < skillCsv->m_repeatTime) {
				client->SetUsingSkill(true);
				_timerQueue.push(SKILL_EVENT(client->GetID(), TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::SwordManHellBlade, {}, _ev.m_power, _ev.m_repeatTime + 1, {}));
			}
			else {
				client->SetUsingSkill(false);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer HellBlade), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::JudgementSword(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManJudgementSword);
			if (_ev.m_repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);

				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

				int id = -2;
				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
						CObjectMgr::GetInstance()->GetClient(bossID)->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						id = bossID;
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(client->GetID(), _ev.m_power, DAMAGE_TYPE::STRENGTH);

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
					_timerQueue.push(SKILL_EVENT(bossID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->m_extraParam2)), EPlayerSkill::SwordManJudgementSword, vec3(static_cast<float>(client->GetMatchNum()), 0.f, 0.f), _ev.m_power + static_cast<int>(client->GetStatus()->GetStat().m_magic * 1.2f), 1, {}, client->GetID()));
				}
				else if (-2 != id) {
					_timerQueue.push(SKILL_EVENT(CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), id)->GetID(), TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->m_extraParam2)), EPlayerSkill::SwordManJudgementSword, vec3(static_cast<float>(client->GetMatchNum()), 0.f, 0.f), _ev.m_power + static_cast<int>(client->GetStatus()->GetStat().m_magic * 1.2f), 1, {}, client->GetID()));
				}
			}
			else if (_ev.m_repeatTime == 1) {
				//pos.x = matchNum
				int objectID;
				vec3 objectPos;
				if (_ev.m_objID >= NPC_ID) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(_ev.m_pos.m_x), _ev.m_objID - NPC_ID);
					objectPos = npc->GetPos() + vec3(0.f, skillCsv->m_posOffset, 0.f);
					objectID = CGameMgr::GetInstance()->JudgeMentSword(static_cast<int>(_ev.m_pos.m_x), objectPos, _ev.m_power, 0, _ev.m_objID, _ev.m_clientID);
				}
				else {
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
					objectPos = client->GetPos() + vec3(0.f, skillCsv->m_posOffset, 0.f);
					objectID = CGameMgr::GetInstance()->JudgeMentSword(static_cast<int>(_ev.m_pos.m_x), objectPos, _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID, _ev.m_clientID);
				}

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(static_cast<int>(_ev.m_pos.m_x))) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_JUDGEMENT_SWORD, objectPos, vec3(0.f, 0.f, 1.f));
				}
			}
			else {
				//power = matchNum
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_ev.m_power)) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(_ev.m_objID, static_cast<SKILL_TYPE>(_ev.m_clientID));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer JudgementSword), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::AnkleCut(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManAnkleCut);
			if (_ev.m_repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				auto slowDebuff = skillCsv->m_debuffInfo[EDebuffType::Slow];

				DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, { 0.f, 0.f, 0.f }, { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());

				CStat changeStat = CStat(0);
				changeStat.m_speed = -slowDebuff.m_debuffValue;


				int bossID = CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())[3];
				if (bossID != -1) {
					if (CObjectMgr::GetInstance()->GetClient(bossID)->GetBoundingBox().Intersects(box)) {
						CObjectMgr::GetInstance()->GetClient(bossID)->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
						CStat stat = CObjectMgr::GetInstance()->GetClient(bossID)->GetStatus()->GetStat();
						stat.m_speed -= slowDebuff.m_debuffValue;
						CObjectMgr::GetInstance()->GetClient(bossID)->GetStatus()->SetStat(stat);
						_timerQueue.push(SKILL_EVENT(bossID, TimeUtil::CurTime(), EPlayerSkill::SwordManAnkleCut, {}, client->GetStatus()->GetStat().m_strength + client->GetStatus()->GetStat().m_magic, 1, {}));
						network::GetInstance()->RegisterTimerEvent({ bossID, TimeUtil::PassedTimeMSec(slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, -1, changeStat });
					}
				}

				for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(client->GetMatchNum(), i);
					if (!npc->m_active)
						continue;
					if (npc->GetBoundingBox().Intersects(box)) {
						npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::STRENGTH);

						float speed = npc->GetSpeed();
						speed -= slowDebuff.m_debuffValue;
						npc->SetSpeed(speed);

						_timerQueue.push(SKILL_EVENT(npc->GetID(), TimeUtil::CurTime(), EPlayerSkill::SwordManAnkleCut, vec3(static_cast<float>(npc->GetMatchNum()), 0.f, 0.f), client->GetStatus()->GetStat().m_strength + client->GetStatus()->GetStat().m_magic, 1, {}, _ev.m_objID));
						network::GetInstance()->RegisterTimerEvent({ npc->GetID(), TimeUtil::PassedTimeMSec(slowDebuff.m_debuffDuration), EVENT_TYPE::EV_STAT_CHANGE, npc->GetMatchNum(), changeStat });
					}
				}
			}
			else {
				auto bleedDebuff = skillCsv->m_debuffInfo[EDebuffType::Bleed];
				if (_ev.m_objID >= NPC_ID) {
					//ev.pos.x = matchNum
					std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(_ev.m_pos.m_x), _ev.m_objID - NPC_ID);
					npc->Damaged(_ev.m_clientID, _ev.m_power, DAMAGE_TYPE::STRENGTH, false);
					if (_ev.m_repeatTime < bleedDebuff.m_debuffRepeatTime)
						_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(bleedDebuff.m_debuffDuration), EPlayerSkill::SwordManAnkleCut, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}));
				}
				else {
					std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
					client->Damage(_ev.m_power, 0, DAMAGE_TYPE::STRENGTH, _ev.m_objID);
					//Damage client
					if (_ev.m_repeatTime < bleedDebuff.m_debuffRepeatTime)
						_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(bleedDebuff.m_debuffDuration), EPlayerSkill::SwordManAnkleCut, {}, _ev.m_power, _ev.m_repeatTime + 1, {}));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer AnkleCut), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::ShieldBash(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			//pos = look
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManShieldBash);

			//Check Collision
			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(client->GetPos(), client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset }, client->GetWorldMatrix());

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

			// Move Client
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
				_timerQueue.push({ _ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::SwordManShieldBash, _ev.m_pos, _ev.m_power, 0, TimeUtil::CurTime() });
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer ShieldBash), " + std::string(ex.what()));
		}
	}

	void CSwordManTimer::ProtectedArea(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			if (_ev.m_repeatTime == 0) {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::SwordManProtectedArea);
				int objectID = CGameMgr::GetInstance()->ProtectedArea(client->GetMatchNum(), _ev.m_pos, _ev.m_power, _ev.m_objID);

				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::SWORDMAN_PROTECTED_AREA, _ev.m_pos, client->GetLook());
				}
				_timerQueue.push(SKILL_EVENT(objectID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->m_extraParam2)), EPlayerSkill::SwordManProtectedArea, {}, client->GetMatchNum(), 1, {}));
			}
			else {
				//power = matchID
				CGameMgr::GetInstance()->ProtectedArea(_ev.m_power, _ev.m_objID);
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSwordManTimer ProtectedArea), " + std::string(ex.what()));
		}
	}
}