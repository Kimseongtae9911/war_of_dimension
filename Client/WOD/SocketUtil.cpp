#include "stdafx.h"
#include "SocketUtil.h"

LPFN_DISCONNECTEX SocketUtil::DisconnectEx = nullptr;
LPFN_CONNECTEX SocketUtil::ConnectEx = nullptr;

void SocketUtil::Startup()
{
	std::wcout.imbue(std::locale("korean"));
	WSADATA wsaData;
	int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (NO_ERROR != error) {
		PrintError("Startup");
		return;
	}
}

void SocketUtil::Cleanup()
{
	WSACleanup();
}

void SocketUtil::PrintError(const char* msg)
{
	WCHAR* mess;
	int errorNum = GetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, errorNum, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&mess, 0, NULL);

	std::cout << msg;
	std::wcout << L"  ¿¡·¯ => " << mess << std::endl;
	while (true);
	LocalFree(mess);
}

int SocketUtil::GetLastError()
{
	return WSAGetLastError();
}