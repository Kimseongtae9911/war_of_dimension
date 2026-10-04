#pragma once

namespace wod_server {

	class LogPrinter
	{
	public:
		static void PrintMsg(const char* msg);
		static void PrintMsg(std::string msg);
	};
}