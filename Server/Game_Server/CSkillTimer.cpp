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

		m_skillFunc.insert({ EPlayerSkill::WizardAttack, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->Attack(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardTeleport, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->Teleport(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardEarthImpact, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->EarthImpact(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardMagicMissile, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->MagicMissile(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardEnergyBall, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->EneryBall(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardDarknessRay, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->DarknessRay(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardMagicEye, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->MagicEye(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardBigBang, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->BigBang(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardBigBangContinue, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->BigBangContinue(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardReflect, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->Reflect(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::WizardOverload, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_wizardTimer->Overload(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::SwordManDodge, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->Dodge(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManHeavySlash, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->HeavySlash(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManAuraBlade, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->AuraBlade(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManHellBlade, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->HellBlade(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManJudgementSword, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->JudgementSword(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManShieldBash, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->ShieldBash(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManProtectedArea, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->ProtectedArea(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::SwordManAnkleCut, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_swordManTimer->AnkleCut(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::FighterDodge, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->Dodge(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterSpinKick, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->SpinKick(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterWildAttack, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->WildAttack(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterWindKick, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->WindKick(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterRisingDragon, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->RisingDragon(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterMeditation, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->Meditation(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterDragonFist, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->DrangonFist(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterIndestructible, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->Indestructible(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterCounter, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->Counter(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::FighterFireBall, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_fighterTimer->FireBall(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::ArcherBackStep, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->BackStep(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherDodge, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->Dodge(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherAttack, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->Attack(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherPenetraitingShot, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->PenetraitingShot(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherStickyArrow, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->StickyArrow(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherPhoenixArrow, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->PhoenixArrow(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherVault, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->Vault(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherStormArrow, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->StormArrow(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherMultipleShot, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->MultipleShot(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ArcherArrowRain, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_archerTimer->ArrowRain(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::OgreAttack, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->Attack(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreHeavySwing, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->HeavySwing(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreCrunch, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->Crunch(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreEndure, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->Endure(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreGluttony, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->Gluttony(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreRockThrow, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->RockThrow(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreButting, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->Butting(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreDimensionCrush, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->DimensionCrush(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::OgreDimensionPunch, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_ogreTimer->DimensionPunch(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::ProgrammerAttack, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->Attack(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerPointer, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->Pointer(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerRelease, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->Release(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerDelete, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->Delete(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerLaser, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->SCL(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerWhileTrue, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->WhileTrue(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::ProgrammerHelloWorld, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {m_proTimer->HelloWorld(_ev, _timerQueue); } });

		m_skillFunc.insert({ EPlayerSkill::Burn, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {Burn(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::MemoryLeak, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {MemoryLeak(_ev, _timerQueue); }});
		m_skillFunc.insert({ EPlayerSkill::Silence, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {Silence(_ev, _timerQueue); } });
		m_skillFunc.insert({ EPlayerSkill::Stun, [this](const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue) {Stun(_ev, _timerQueue); } });
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
				if (ev.m_wakeUpTime > current_time) {
					m_timerQueue.push(ev);
					std::this_thread::yield();
					continue;
				}
				auto iter = m_skillFunc.find(ev.m_skillType);
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

	void CSkillTimer::Burn(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::Burn);
			if (_ev.m_objID >= NPC_ID) {
				//ev.pos.x = matchNum
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(_ev.m_pos.m_x), _ev.m_objID - NPC_ID);
				if (npc->m_active)
					npc->Damaged(_ev.m_clientID, static_cast<int>(skillCsv->m_extraParam1) * _ev.m_power, DAMAGE_TYPE::MAGIC, false);
			}
			else {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				client->Damage(_ev.m_power, 0, DAMAGE_TYPE::MAGIC, _ev.m_clientID);
			}
			if (_ev.m_repeatTime < skillCsv->m_repeatTime)
				_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::Burn, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}, _ev.m_clientID));
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer Burn), " + std::string(ex.what()));
		}
	}

	void CSkillTimer::MemoryLeak(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			auto skillCsv = SkillCsvMgr::GetInstance()->GetSkillCsv(EPlayerSkill::Burn);
			if (_ev.m_objID >= NPC_ID) {
				//ev.pos.x = matchNum
				std::shared_ptr<CNpc> npc = CObjectMgr::GetInstance()->GetNpc(static_cast<int>(_ev.m_pos.m_x), _ev.m_objID - NPC_ID);
				npc->Damaged(_ev.m_clientID, _ev.m_power, DAMAGE_TYPE::MAGIC, false);
				if (_ev.m_repeatTime < skillCsv->m_repeatTime && npc->m_active)
					_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::MemoryLeak, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}, _ev.m_clientID));
			}
			else {
				std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
				client->Damage(_ev.m_power, 0, DAMAGE_TYPE::MAGIC, _ev.m_clientID);
				if (_ev.m_repeatTime < skillCsv->m_repeatTime)
					_timerQueue.push(SKILL_EVENT(_ev.m_objID, TimeUtil::PassedTimeMSec(skillCsv->m_damageCycleTime), EPlayerSkill::MemoryLeak, _ev.m_pos, _ev.m_power, _ev.m_repeatTime + 1, {}, _ev.m_clientID));
			}
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CSkillTimer MemoryLeak), " + std::string(ex.what()));
		}
	}

	void CSkillTimer::Silence(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			if (client->GetStatus()->m_skillBuff == SKILL_BUFF::SILENCE) {
				client->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;
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

	void CSkillTimer::Stun(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)
	{
		try {
			std::shared_ptr<CClient> client = CObjectMgr::GetInstance()->GetClient(_ev.m_objID);
			if (client->GetStatus()->m_skillBuff == SKILL_BUFF::STUN) {
				client->GetStatus()->m_skillBuff = SKILL_BUFF::NONE;
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