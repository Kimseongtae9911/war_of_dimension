#pragma once
#include <ServerCore/Session.h>
#include "Resource.h"
namespace wod_server {
class SocketUtil {
public:
    static void Startup() { wod::core::TransportHost::Start(); }
    static void Cleanup() { wod::core::TransportHost::Stop(); }
    static wod::core::IocpService& Runtime() { return wod::core::TransportHost::Get(); }
    static void PrintError(const char* _op);
    static int GetLastError() { return WSAGetLastError(); }
    static CSessionPool m_socketpool;
};
}
