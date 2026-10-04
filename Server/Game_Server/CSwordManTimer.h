#pragma once

namespace wod_server {
	class CSwordManTimer
	{
	public:
		CSwordManTimer() {}
		~CSwordManTimer() {}

		void Dodge(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void HeavySlash(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void AuraBlade(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void HellBlade(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void JudgementSword(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void AnkleCut(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void ShieldBash(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void ProtectedArea(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
	};
}