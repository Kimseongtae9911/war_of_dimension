#pragma once

namespace wod_server {
	class Session;

	class CSessionPool {
	public:
		void push(const std::shared_ptr<Session>& session);
		bool try_pop(std::shared_ptr<Session>& session);

	private:
		concurrency::concurrent_priority_queue<std::shared_ptr<Session>> sessionPool;
	};

	class Resource
	{
	public:
		static OverlapEx* GetOverObjectFromPool();

		static CSessionPool sessionPool;
		static concurrency::concurrent_priority_queue<OverlapEx*> overExPool;
	};

}