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
        bool m_matchReady = false;
        bool m_packetReady = false;

    };

}
