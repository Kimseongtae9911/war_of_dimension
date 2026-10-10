#pragma once

namespace wod_server {
	class CProTimer
	{
	public:
		CProTimer() {}
		~CProTimer() {}

		void Attack(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Pointer(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Release(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void Delete(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void SCL(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void WhileTrue(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
		void HelloWorld(const SKILL_EVENT& _ev, concurrency::concurrent_priority_queue<SKILL_EVENT>& _timerQueue);
	};
}