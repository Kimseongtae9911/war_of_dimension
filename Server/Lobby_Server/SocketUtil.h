#pragma once
#include <ServerCore/Session.h>
namespace wod_server {
class SocketUtil {
public:
    static void Startup() { wod::core::TransportHost::Start(); }
    static void Cleanup() { wod::core::TransportHost::Stop(); }
    static wod::core::IocpService& Runtime() { return wod::core::TransportHost::Get(); }
    static void PrintError(const char* op);
    static int GetLastError() { return WSAGetLastError(); }
};
}
