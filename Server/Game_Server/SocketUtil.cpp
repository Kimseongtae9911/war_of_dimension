#include "pch.h"
#include "SocketUtil.h"
namespace wod_server {
CSessionPool SocketUtil::socketpool;
void SocketUtil::PrintError(const char* op) { LogPrinter::PrintMsg(std::string(op) + ": " + std::to_string(WSAGetLastError())); }
}
