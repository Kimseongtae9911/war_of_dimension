#pragma once

namespace wod_server {

	struct SocketResource {
		SocketResource() {}
		SocketResource(const SOCKET& s) : socket(s) { entryTime = std::chrono::system_clock::now(); }

		SOCKET socket;
		std::chrono::system_clock::time_point entryTime;

		bool operator<(const SocketResource& other) const {
			return entryTime > other.entryTime;
		}
	};

	class CSocketPool {
	public:
		void push(const SOCKET& socket) { m_socketpool.push(SocketResource(socket)); }
		bool try_pop(SocketResource& socket) { return m_socketpool.try_pop(socket); }

	private:
		concurrency::concurrent_priority_queue<SocketResource> m_socketpool;
	};

	class Resource
	{
	public:
		static OverlapEx* GetOverObjectFromPool();

		static CSocketPool socketpool;
		static concurrency::concurrent_priority_queue<OverlapEx*> overExPool;
	};

}