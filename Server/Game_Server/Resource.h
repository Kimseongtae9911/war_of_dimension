#pragma once
#include <ServerCore/Concurrency.h>

namespace wod_server {
	class Session;

	class CSessionPool {
	public:
		void push(const std::shared_ptr<Session>& _session);
		bool try_pop(std::shared_ptr<Session>& _session);

	private:
		concurrency::concurrent_priority_queue<std::shared_ptr<Session>> m_sessionPool;
	};

	class Resource
	{
	public:
		static OverlapEx* GetOverObjectFromPool();

		static CSessionPool m_sessionPool;
		static wod::core::ObjectPool<OverlapEx> m_overExPool;
	};

}