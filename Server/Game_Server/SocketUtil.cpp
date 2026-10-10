#include "pch.h"
#include "SocketUtil.h"
namespace wod_server {
CSessionPool SocketUtil::m_socketpool;
void SocketUtil::PrintError(const char* _op) { LogPrinter::PrintMsg(std::string(_op) + ": " + std::to_string(WSAGetLastError())); }
}
