#pragma once

namespace wod_server {

	class LogPrinter
	{
	public:
		static void PrintMsg(const char* msg);
		static void PrintMsg(std::string msg);

		template<class T>
		static void PrintMsg(T msg);

		template<class T>
		static void PrintMsg(std::string str, T msg);

	public:
		static std::mutex printlock;
	};

	template<class T>
	inline void LogPrinter::PrintMsg(T msg)
	{
		printlock.lock();
		std::cout << msg << std::endl;
		printlock.unlock();
	}

	template<class T>
	inline void LogPrinter::PrintMsg(std::string str, T msg)
	{
		printlock.lock();
		std::cout << str << msg << std::endl;
		printlock.unlock();
	}

}