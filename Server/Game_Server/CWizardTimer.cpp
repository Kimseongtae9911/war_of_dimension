#include "pch.h"
#include "CWizardTimer.h"
#include "CClient.h"
#include "ClientInfos.h"


namespace wod_server {
	void CWizardTimer::Attack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->WizardAttack(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_ATTACK, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Attack), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Teleport(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);


			if (client->GetUsingSkill()) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::NextFrameTime(), EPlayerSkill::WizardTeleport, ev.pos, 0, 0, {}));
			}
			else {
				client->SetPos(ev.pos);
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMovePacket(client->GetMatchId(), client->GetPos(), client->GetDir());
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Teleport), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::EarthImpact(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardEarthImpact);

			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)[3];
			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, client->GetID());
				}
			}

			//Minion
			for (int i = 0; i < MAX_MINION; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					npc->Damaged(ev.objID, ev.power, DAMAGE_TYPE::MAGIC);
				}
			}

			//Monster
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
			LogPrinter::PrintMsg("Err(CWizardTimer EarthImpact), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::MagicMissile(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->MagicMissle(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_MAGIC_MISSILE, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer MagicMissile), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::EneryBall(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->EnergyBall(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_ENERGY_BALL, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer EneryBall), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::DarknessRay(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardDarknessRay);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(ev.pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->posOffset * 2.f }, client->GetWorldMatrix());
			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), ev.power, client->GetStatus()->GetStat().critical, DAMAGE_TYPE::MAGIC, ev.objID, false);

			if (ev.repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::WIZARD_DARKNESS_RAY, client->GetPos() + client->GetLook() * 3.f + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::WizardDarknessRay, ev.pos, ev.power, ev.repeatTime + 1, {}));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (id == -1)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(0, SKILL_TYPE::WIZARD_DARKNESS_RAY);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer DarknessRay), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::MagicEye(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardMagicEye);

			if (ev.repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMagicEyePacket(true);
				}
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->extraParam1)), EPlayerSkill::WizardMagicEye, ev.pos, 0, 1, {}, client->GetMatchNum()));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMagicEyePacket(false);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer MagicEye), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::BigBang(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			int objectID = CGameMgr::GetInstance()->BigBang(client->GetMatchNum(), ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), ev.power, client->GetStatus()->GetStat().critical, ev.objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_BIGBANG, ev.pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer BigBang), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::BigBangContinue(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(ev.objID)[3];
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardBigBang);

			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(ev.power, 0, DAMAGE_TYPE::MAGIC, ev.clientID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(ev.objID, i);
				if (!npc->active)
					continue;
				if (DistanceXZ(npc->GetPos(), ev.pos) <= skillCsv->skillRadius) {
					npc->Damaged(ev.clientID, ev.power, DAMAGE_TYPE::MAGIC, false);
				}
			}

			if (ev.repeatTime < skillCsv->extraParam1) {
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::WizardBigBangContinue, ev.pos, ev.power, ev.repeatTime + 1, {}, ev.clientID));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(ev.objID)) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(CObjectMgr::GetInstance()->GetClient(ev.clientID)->GetMatchId(), SKILL_TYPE::WIZARD_BIGBANG_CONTINUE);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer BigBangContinue), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Reflect(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			CObjectMgr::GetInstance()->GetClient(ev.objID)->GetStatus()->defensiveBuff = DEFENSIVE_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Reflect), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Overload(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			CObjectMgr::GetInstance()->GetClient(ev.objID)->GetStatus()->coolTimeBuff = COOLTIME_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Overload), " + std::string(ex.what()));
		}
	}
}