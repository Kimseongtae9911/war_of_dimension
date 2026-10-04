#pragma once

namespace wod_server {
	class TCPSocket;

	class SocketUtil
	{
	public:
		static LPFN_DISCONNECTEX DisconnectEx;

		static void Startup();
		static void Cleanup();

		static void PrintError(const char* op);
		static int GetLastError();
	};
}