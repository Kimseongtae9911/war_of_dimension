#pragma once
#include <ServerCore/Concurrency.h>

namespace wod_server {

	struct SocketResource {
		SocketResource() {}
		SocketResource(const SOCKET& _s) : m_socket(_s) { m_entryTime = std::chrono::system_clock::now(); }

		SOCKET m_socket;
		std::chrono::system_clock::time_point m_entryTime;

		bool operator<(const SocketResource& _other) const {
			return m_entryTime > _other.m_entryTime;
		}
	};

	class CSocketPool {
	public:
		void push(const SOCKET& _socket) { m_socketpool.push(SocketResource(_socket)); }
		bool try_pop(SocketResource& _socket) { return m_socketpool.try_pop(_socket); }

	private:
		concurrency::concurrent_priority_queue<SocketResource> m_socketpool;
	};

	class Resource
	{
	public:
		static OverlapEx* GetOverObjectFromPool();

		static CSocketPool m_socketpool;
		static wod::core::ObjectPool<OverlapEx> m_overExPool;
	};

}