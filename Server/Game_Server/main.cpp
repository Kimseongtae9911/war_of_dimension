#include "pch.h"
#include "CServer.h"

int main()
{
	wod_server::CServer* server = new wod_server::CServer();
	server->Initialize();

	// Main Loop
	server->Run();

	server->Release();

	return 0;
}