#include "pch.h"
#include "LogUtil.h"

namespace wod_server {

	void LogPrinter::PrintMsg(const char* msg)
	{
		std::cout << msg << std::endl;
	}

	void LogPrinter::PrintMsg(std::string msg)
	{
		std::cout << msg << std::endl;
	}
}
