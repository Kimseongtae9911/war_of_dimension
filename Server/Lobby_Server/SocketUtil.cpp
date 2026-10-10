#include "pch.h"
#include "SocketUtil.h"
namespace wod_server {
void SocketUtil::PrintError(const char* _op) { LogPrinter::PrintMsg(std::string(_op) + ": " + std::to_string(WSAGetLastError())); }
}
