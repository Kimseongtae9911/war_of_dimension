#include "pch.h"
#include "CSkillTimer.h"
#include "CClient.h"
#include "CNpc.h"
#include "ClientInfos.h"

namespace wod_server {

	CSkillTimer::CSkillTimer()
	{
		m_archerTimer = new CArcherTimer();
		m_fighterTimer = new CFigtherTimer();
		m_swordManTimer = new CSwordManTimer();
		m_wizardTimer = new CWizardTimer();
		m_ogreTimer = new COgreTimer();
		m_proTimer = new CProTimer();

		m_skillFunc.insert({ EPlayerSkill::WizardAttack, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->Attack(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardTeleport, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->Teleport(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardEarthImpact, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->EarthImpact(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardMagicMissile, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->MagicMissile(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardEnergyBall, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->EneryBall(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardDarknessRay, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->DarknessRay(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardMagicEye, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->MagicEye(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardBigBang, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->BigBang(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardBigBangContinue, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->BigBangContinue(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardReflect, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->Reflect(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardOverload, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_wizardTimer->Overload(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::SwordManDodge, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->Dodge(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManHeavySlash, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->HeavySlash(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManAuraBlade, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->AuraBlade(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManHellBlade, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->HellBlade(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManJudgementSword, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->JudgementSword(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManShieldBash, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->ShieldBash(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManProtectedArea, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->ProtectedArea(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManAnkleCut, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_swordManTimer->AnkleCut(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::FighterDodge, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->Dodge(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterSpinKick, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->SpinKick(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterWildAttack, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->WildAttack(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterWindKick, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->WindKick(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterRisingDragon, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->RisingDragon(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterMeditation, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->Meditation(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterDragonFist, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->DrangonFist(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterIndestructible, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->Indestructible(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterCounter, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->Counter(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterFireBall, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_fighterTimer->FireBall(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::ArcherBackStep, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->BackStep(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherDodge, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->Dodge(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherAttack, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->Attack(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherPenetraitingShot, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->PenetraitingShot(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherStickyArrow, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->StickyArrow(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherPhoenixArrow, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->PhoenixArrow(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherVault, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->Vault(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherStormArrow, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->StormArrow(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherMultipleShot, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->MultipleShot(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherArrowRain, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_archerTimer->ArrowRain(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::OgreAttack, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->Attack(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreHeavySwing, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->HeavySwing(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreCrunch, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->Crunch(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreEndure, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->Endure(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreGluttony, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->Gluttony(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreRockThrow, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->RockThrow(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreButting, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->Butting(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreDimensionCrush, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->DimensionCrush(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreDimensionPunch, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_ogreTimer->DimensionPunch(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::ProgrammerAttack, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->Attack(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerPointer, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->Pointer(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerRelease, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->Release(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerDelete, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->Delete(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerLaser, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->SCL(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerWhileTrue, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->WhileTrue(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerHelloWorld, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {m_proTimer->HelloWorld(ev, timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::Burn, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {Burn(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::MemoryLeak, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {MemoryLeak(ev, timerQueue); }});
		m_skillFunc.insert({ EPlayerSkill::Silence, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {Silence(ev, timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::Stun, [this](const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue) {Stun(ev, timerQueue); } });
	}

	CSkillTimer::~CSkillTimer()
	{
		delete m_archerTimer;
		delete m_fighterTimer;
		delete m_swordManTimer;
		delete m_wizardTimer;
		delete m_ogreTimer;
		delete m_proTimer;
	}

	void CSkillTimer::Run()
	{
		while (true) {
			SKILL_EVENT ev;
			auto current_time = TimeUtil::CurTime();
			if (m_timerQueue.try_pop(ev)) {
				if (ev.wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::yield();
					continue;
				}
				auto iter = m_skillFunc.find(ev.skillType);
				if (iter != m_skillFunc.end()) {
					iter->second(ev, m_timerQueue);
				}
				else
					LogPrinter::PrintMsg("Wrong Skill Type In Skill Timer");
				continue;
			}
			std::this_thread::yield();
		}
	}

	void CSkillTimer::Burn(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::Burn);
			if (ev.objID >= NPC_ID) {
				//ev.pos.x = matchNum
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(ev.pos.x), ev.objID - NPC_ID);
				if (npc->active)
					npc->Damaged(ev.clientID, static_cast<int>(skillCsv->extraParam1) * ev.power, DAMAGE_TYPE::MAGIC, false);
			}
			else {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				client->Damage(ev.power, 0, DAMAGE_TYPE::MAGIC, ev.clientID);
			}
			if (ev.repeatTime < skillCsv->repeatTime)
				timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::Burn, ev.pos, ev.power, ev.repeatTime + 1, {}, ev.clientID));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer Burn), " + std::string(ex.what()));
		}
	}

	void CSkillTimer::MemoryLeak(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::Burn);
			if (ev.objID >= NPC_ID) {
				//ev.pos.x = matchNum
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(ev.pos.x), ev.objID - NPC_ID);
				npc->Damaged(ev.clientID, ev.power, DAMAGE_TYPE::MAGIC, false);
				if (ev.repeatTime < skillCsv->repeatTime && npc->active)
					timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::MemoryLeak, ev.pos, ev.power, ev.repeatTime + 1, {}, ev.clientID));
			}
			else {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
				client->Damage(ev.power, 0, DAMAGE_TYPE::MAGIC, ev.clientID);
				if (ev.repeatTime < skillCsv->repeatTime)
					timerQueue.push(SKILL_EVENT(ev.objID, TimeUtil::PassedTimeMSec(skillCsv->damageCycleTime), EPlayerSkill::MemoryLeak, ev.pos, ev.power, ev.repeatTime + 1, {}, ev.clientID));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer MemoryLeak), " + std::string(ex.what()));
		}
	}

	void CSkillTimer::Silence(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			if (client->GetStatus()->skillBuff == SKILL_BUFF::SILENCE) {
				client->GetStatus()->skillBuff = SKILL_BUFF::NONE;
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::NONE));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer Silence), " + std::string(ex.what()));
		}
	}

	void CSkillTimer::Stun(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(ev.objID);
			if (client->GetStatus()->skillBuff == SKILL_BUFF::STUN) {
				client->GetStatus()->skillBuff = SKILL_BUFF::NONE;
				for (int id : CMatchMgr::GetInstance()->GetMatchPlayers(client->GetMatchNum())) {
					if (-1 == id)
						continue;
					CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()->SendPlayerStatusChangePakcet(client->GetMatchId(), 0, static_cast<short>(SKILL_BUFF::NONE));
				}
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer Stun), " + std::string(ex.what()));
		}
	}
}