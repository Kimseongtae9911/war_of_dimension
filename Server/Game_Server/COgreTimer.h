#pragma once

namespace wod_server {
	class COgreTimer
	{
	public:
		COgreTimer() {}
		~COgreTimer() {}

		void Attack(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void HeavySwing(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void Crunch(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void Endure(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void RockThrow(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void Butting(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void DimensionCrush(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void DimensionPunch(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
		void Gluttony(const SKILL_EVENT& ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& timerQueue);
	};
}
