#pragma once

namespace wod_server {

    class CServer
    {
    public:
        bool Initialize();
        bool Release();
        void Run();

    private:
        std::vector<std::thread> m_iocpThreads;
        std::vector<std::thread> m_workerThreads;
    };

}
