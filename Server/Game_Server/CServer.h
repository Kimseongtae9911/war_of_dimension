#pragma once

namespace wod_server {

	class CServer
	{
	public:
		bool Initialize();

		bool Release();
		void Run();

    private:
        bool m_transportReady = false;
        bool m_skillCsvReady = false;
        bool m_npcCsvReady = false;
        bool m_itemCsvReady = false;
        bool m_gameReady = false;
        bool m_packetReady = false;
        bool m_matchReady = false;
        bool m_skillFactoryReady = false;

	};

}