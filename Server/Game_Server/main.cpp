#include "pch.h"
#include "CServer.h"
int main() {
    wod::core::ConfigureProcessDiagnostics();
    wod_server::CServer server;
    try {
        if (!server.Initialize()) { server.Release(); return 1; }
        server.Run(); return server.Release() ? 0 : 1;
    } catch(const std::exception& error) {
        wod_server::LogPrinter::PrintMsg(error.what());
        try { server.Release(); } catch(const std::exception& cleanup) { wod_server::LogPrinter::PrintMsg(cleanup.what()); }
        return 1;
    }
}
