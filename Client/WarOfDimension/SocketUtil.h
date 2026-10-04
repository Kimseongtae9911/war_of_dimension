#pragma once

class TCPSocket;

class SocketUtil
{
public:
	static LPFN_DISCONNECTEX DisconnectEx;
	static LPFN_CONNECTEX ConnectEx;

	static void Startup();
	static void Cleanup();

	static void PrintError(const char* op);
	static int GetLastError();
};