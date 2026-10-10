#include "pch.h"
#include "CWizardTimer.h"
#include "CClient.h"
#include "ClientInfos.h"


namespace wod_server {
	void CWizardTimer::Attack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->WizardAttack(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_ATTACK, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Attack), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Teleport(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);


			if (client->GetUsingSkill()) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::NextFrameTime(), EPlayerSkill::WizardTeleport, _ev.m_pos, 0, 0, {}));
			}
			else {
				client->SetPos(_ev.m_pos);
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

	void CWizardTimer::EarthImpact(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int matchNum = client->GetMatchNum();
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardEarthImpact);

			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(matchNum)[3];
			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(_ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, client->GetID());
				}
			}

			//Minion
			for (int i = 0; i < MAX_MINION; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(matchNum, i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_objID, _ev.m_power, DAMAGE_TYPE::MAGIC);
				}
			}

			//Monster
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
			LogPrinter::PrintMsg("Err(CWizardTimer EarthImpact), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::MagicMissile(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->MagicMissle(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_MAGIC_MISSILE, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer MagicMissile), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::EneryBall(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->EnergyBall(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_ENERGY_BALL, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer EneryBall), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::DarknessRay(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardDarknessRay);

			DirectX::BoundingOrientedBox box = GameUtil::GenerateShortRangeBox(_ev.m_pos, client->GetLook(), { client->GetBoundingBox().Extents }, { 2.f, 2.f, skillCsv->m_posOffset * 2.f }, client->GetWorldMatrix());
			GameUtil::HeroSkillCollisionCheck(box, client->GetMatchNum(), _ev.m_power, client->GetStatus()->GetStat().m_critical, DAMAGE_TYPE::MAGIC, _ev.m_objID, false);

			if (_ev.m_repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(0, SKILL_TYPE::WIZARD_DARKNESS_RAY, client->GetPos() + client->GetLook() * 3.f + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
				}
			}

			if (client->GetUsingSkill()) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::WizardDarknessRay, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}));
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

	void CWizardTimer::MagicEye(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardMagicEye);

			if (_ev.m_repeatTime == 0) {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendMagicEyePacket(true);
				}
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(static_cast<int>(skillCsv->m_extraParam1)), EPlayerSkill::WizardMagicEye, _ev.m_pos, 0, 1, {}, client->GetMatchNum()));
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

	void CWizardTimer::BigBang(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			int objectID = CGameMgr::GetInstance()->BigBang(client->GetMatchNum(), _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook(), _ev.m_power, client->GetStatus()->GetStat().m_critical, _ev.m_objID);

			for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
				if (-1 == id)
					continue;
				CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendAddSkillObjectPacket(objectID, SKILL_TYPE::WIZARD_BIGBANG, _ev.m_pos + ClientInfos::HERO_ATTACK_OFFSET, client->GetLook());
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer BigBang), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::BigBangContinue(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			int bossId = CMatchMgr::GetInstance()->GetMatchPlayers(_ev.m_objID)[3];
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::WizardBigBang);

			if (-1 != bossId) {
				if (DistanceXZ(CObjectMgr::GetInstance()->GetClient(bossId)->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					CObjectMgr::GetInstance()->GetClient(bossId)->Damage(_ev.m_power, 0, DAMAGE_TYPE::MAGIC, _ev.m_clientID);
				}
			}

			for (int i = 0; i < MAX_MINION + MONSTER_NUM; ++i) {
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(_ev.m_objID, i);
				if (!npc->m_active)
					continue;
				if (DistanceXZ(npc->GetPos(), _ev.m_pos) <= skillCsv->m_skillRadius) {
					npc->Damaged(_ev.m_clientID, _ev.m_power, DAMAGE_TYPE::MAGIC, false);
				}
			}

			if (_ev.m_repeatTime < skillCsv->m_extraParam1) {
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::WizardBigBangContinue, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}, _ev.m_clientID));
			}
			else {
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(_ev.m_objID)) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendRemoveSkillObjectPacket(CObjectMgr::GetInstance()->GetClient(_ev.m_clientID)->GetMatchId(), SKILL_TYPE::WIZARD_BIGBANG_CONTINUE);
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer BigBangContinue), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Reflect(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			CObjectMgr::GetInstance()->GetClient(_ev.m_objID)->GetStatus()->m_defensiveBuff = DEFENSIVE_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Reflect), " + std::string(ex.what()));
		}
	}

	void CWizardTimer::Overload(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			CObjectMgr::GetInstance()->GetClient(_ev.m_objID)->GetStatus()->m_coolTimeBuff = COOLTIME_BUFF::NONE;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CWizardTimer Overload), " + std::string(ex.what()));
		}
	}
}