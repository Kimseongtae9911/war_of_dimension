#pragma once
#include "CArcherTimer.h"
#include "CSwordManTimer.h"
#include "CFigtherTimer.h"
#include "CWizardTimer.h"
#include "COgreTimer.h"
#include "CProTimer.h"

namespace wod_server {
	class CSkillTimer
	{
	public:
		CSkillTimer();
		~CSkillTimer();

		void PushEvent(const SKILL_EVENT& _ev) { m_timerQueue.push(_ev); }

		void Run();

	private:
		concurrency::concurrent_priority_queue<SKILL_EVENT> m_timerQueue;
		std::unordered_map<EPlayerSkill, std::function<void(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue)>> m_skillFunc;

		CArcherTimer* m_archerTimer;
		CFigtherTimer* m_fighterTimer;
		CSwordManTimer* m_swordManTimer;
		CWizardTimer* m_wizardTimer;
		COgreTimer* m_ogreTimer;
		CProTimer* m_proTimer;

	private:
		void Burn(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void MemoryLeak(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Silence(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Stun(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
	};

}