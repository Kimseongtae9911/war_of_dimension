#pragma once

namespace wod_server {
	class CFigtherTimer
	{
	public:
		CFigtherTimer() {}
		~CFigtherTimer() {}

		void Dodge(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void SpinKick(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void WildAttack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void WindKick(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void RisingDragon(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Meditation(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void DrangonFist(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Indestructible(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Counter(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void FireBall(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
	};
}