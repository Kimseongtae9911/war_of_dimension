#pragma once

namespace wod_server {

	class CServer
	{
	public:
		bool Initialize();
		bool Release();
		void Run();

	private:
		void TimerFunc();		

	private:
		std::vector<std::thread> m_iocpThread;
	};

}