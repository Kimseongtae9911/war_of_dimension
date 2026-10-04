#include "pch.h"
#include "CServer.h"

int main()
{
	std::wcout.imbue(std::locale("korean"));

	wod_server::CServer server;

	// Initialize Server
	if (!server.Initialize()) {
		wod_server::LogPrinter::PrintMsg("Server Initialize Failed");
		exit(1);
	}

	// Main Loop
	server.Run();

	// Release Memory
	if (!server.Release()) {
		wod_server::LogPrinter::PrintMsg("Server Release Failed");
		exit(1);
	}

	return 0;
}