#pragma once

namespace wod_server {
	class Session;

	class SocketUtil
	{
	public:
		static LPFN_DISCONNECTEX DisconnectEx;

		static void Startup();
		static void Cleanup();

		static void PrintError(const char* op);
		static int GetLastError();

		static concurrency::concurrent_priority_queue<std::shared_ptr<Session>> socketpool;
	};
}