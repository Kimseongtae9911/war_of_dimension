#include "pch.h"
#include "LogUtil.h"

namespace wod_server {
	std::mutex LogPrinter::printlock;

	void LogPrinter::PrintMsg(const char* msg)
	{
		printlock.lock();
		std::cerr << msg << std::endl;
		printlock.unlock();
	}

	void LogPrinter::PrintMsg(std::string msg)
	{
		printlock.lock();
		std::cerr << msg << std::endl;
		printlock.unlock();
	}
}