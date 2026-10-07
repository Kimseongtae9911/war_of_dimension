#pragma once
#include <iostream>
#include <mutex>
#include <string>
#include <cstdlib>
#ifdef _DEBUG
#include <crtdbg.h>
#endif

namespace wod::core {
inline void ConfigureProcessDiagnostics() {
    _set_error_mode(_OUT_TO_STDERR);
    _set_abort_behavior(0, _CALL_REPORTFAULT);
#ifdef _DEBUG
    for (int type : {_CRT_WARN, _CRT_ERROR, _CRT_ASSERT}) {
        _CrtSetReportMode(type, _CRTDBG_MODE_FILE);
        _CrtSetReportFile(type, _CRTDBG_FILE_STDERR);
    }
#endif
}
class LogPrinter {
public:
    inline static std::mutex printlock;
    template<class T> static void PrintMsg(const T& msg) {
        std::lock_guard lock(printlock); std::cerr << msg << '\n';
    }
    template<class T> static void PrintMsg(const std::string& prefix, const T& msg) {
        std::lock_guard lock(printlock); std::cerr << prefix << msg << '\n';
    }
};
}
