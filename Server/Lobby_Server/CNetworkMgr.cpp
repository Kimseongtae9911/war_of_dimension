#include "pch.h"
#include <Protocol/Validation.h>
#include "CNetworkMgr.h"
#include "CMatchMgr.h"
#include "TCPSocket.h"

namespace wod_server
{
std::unique_ptr<CNetworkMgr> CNetworkMgr::m_instance;

bool CNetworkMgr::Initialize()
{
#ifndef LOCAL_TEST
    std::cout << "Input Game Server IP: ";
    std::cin >> gameIP;
#else
    m_gameIP = "127.0.0.1";
#endif

#ifdef WITH_DATABASE
    m_dataBaseThread = new CDataBaseThread();
#endif
    m_handle = std::make_shared<TCPSocket>();
    m_gameServer = std::make_shared<Session>(true);

    m_iocpfunc.insert({OP_TYPE::OP_SERVER_CONNECT, [this](int _id, int _bytes, OverlapEx* _over_ex) {
                           ServerConnect(_id, _bytes, _over_ex);
                       }});
    m_iocpfunc.insert({OP_TYPE::OP_ACCEPT, [this](int _id, int _bytes, OverlapEx* _over_ex) {
                           CNetworkMgr::Accept(_id, _bytes, _over_ex);
                       }});
    m_iocpfunc.insert({OP_TYPE::OP_RECV, [this](int _id, int _bytes, OverlapEx* _over_ex) {
                           CNetworkMgr::Recv(_id, _bytes, _over_ex);
                       }});
    m_iocpfunc.insert({OP_TYPE::OP_SEND, [this](int _id, int _bytes, OverlapEx* _over_ex) {
                           CNetworkMgr::Send(_id, _bytes, _over_ex);
                       }});
    m_iocpfunc.insert({OP_TYPE::OP_DISCONNECT, [this](int _id, int _bytes, OverlapEx* _over_ex) {
                           CNetworkMgr::Disconnect(_id, _bytes, _over_ex);
                       }});
    ;

    // Make Socket Pool
    for (int i = 0; i < MAX_SOCKET; ++i)
    {
        SOCKET s = NetworkRuntime::Get().CreateSocket();
        Resource::m_socketpool.push(s);

        //Register socket to iocp, Create Client Objects
        int id = static_cast<int>(s);
        NetworkRuntime::Get().Attach(s, id);
        CUserMgr::GetInstance()->MakeClientObject(id);
    }

    for (int i = 0; i < MAX_OVEREX_OBJECT; ++i)
    {
        OverlapEx* overEx = new OverlapEx;
        Resource::m_overExPool.push(overEx);
    }

    m_handle->Bind(SockAddr(LOBBY_PORT));
    m_handle->Listen();

    SocketResource socketResource;
    Resource::m_socketpool.try_pop(socketResource);
    m_handle->Accept(socketResource.m_socket);

    m_clientNum = 0;
    m_gameseverConnected = false;

    m_p2pNetwork = new CP2PNetwork;

    return true;
}

bool CNetworkMgr::Release()
{
    PrepareStop();
    NetworkRuntime::Get().RequestStop(1);
    wod::core::Completion completion;
    while (NetworkRuntime::Get().Stats().m_pending && NetworkRuntime::Get().Poll(completion))
    {
        auto op = completion.m_context->m_operation;
        if (op == wod::core::IoOperation::Send || op == wod::core::IoOperation::Disconnect || op == wod::core::IoOperation::AppEvent)
            Resource::m_overExPool.push(static_cast<OverlapEx*>(completion.m_context));
    }
    NetworkRuntime::Get().Finish();
    LogPrinter::PrintMsg("ServerCore stop pending=0 sockets=0 leased=" + std::to_string(Resource::m_overExPool.Leased()));
    Resource::m_overExPool.Clear();
    SocketResource socket;
    while (Resource::m_socketpool.try_pop(socket))
    {
    }
    delete m_p2pNetwork;
    m_p2pNetwork = nullptr;
#ifdef WITH_DATABASE
    delete m_dataBaseThread;
    m_dataBaseThread = nullptr;
#endif
    m_gameServer.reset();
    m_handle.reset();
    return true;
}

void CNetworkMgr::IOCPFunc()
{

    try
    {
        wod::core::Completion completion;
        while (NetworkRuntime::Get().Poll(completion))
        {
            auto* over = static_cast<OverlapEx*>(completion.m_context);
            const auto operation = over->m_operation;
            if (m_stopping.load() || NetworkRuntime::Get().IsStopping())
            {
                if (operation == wod::core::IoOperation::Send || operation == wod::core::IoOperation::Disconnect || operation == wod::core::IoOperation::AppEvent)
                    Resource::m_overExPool.push(over);
                continue;
            }
            if (completion.m_error || (operation == wod::core::IoOperation::Receive && completion.m_bytes == 0))
            {
                if (operation == wod::core::IoOperation::Send)
                {
                    Resource::m_overExPool.push(over);
                    continue;
                }
                if (operation == wod::core::IoOperation::Accept)
                {
                    m_workerFailed.store(true);
                    LogPrinter::PrintMsg("Accept failed: " + std::to_string(completion.m_error));
                    wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
                    continue;
                }
                if (operation != wod::core::IoOperation::Disconnect)
                {
                    if (completion.m_key == m_gameServer->GetSocket())
                    {
                        LogPrinter::PrintMsg("Server link closed");
                        continue;
                    }
                    CUserMgr::GetInstance()->DisconnectClient(static_cast<int>(completion.m_key));
                    continue;
                }
            }

            if (completion.m_context->m_operation == wod::core::IoOperation::Accept && !m_gameseverConnected)
            {
                m_gameseverConnected = true;
                over->SetOP(OP_TYPE::OP_SERVER_CONNECT);
            }

            auto found = m_iocpfunc.find(over->GetOP());
            if (found != m_iocpfunc.end())
                found->second(static_cast<int>(completion.m_key), static_cast<int>(completion.m_bytes), over);
            else
            {
                LogPrinter::PrintMsg("Unknown application completion");
                if (operation == wod::core::IoOperation::AppEvent)
                    Resource::m_overExPool.push(over);
            }
        }
    }
    catch (const std::exception& error)
    {
        m_workerFailed.store(true);
        LogPrinter::PrintMsg(std::string("IOCP worker failed: ") + error.what());
        wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
    }
}

void CNetworkMgr::TimerFunc()
{
    try
    {
        std::unique_lock lock(m_timerMutex);
        while (!m_stopping.load())
        {
            if (m_timerQueue.empty())
            {
                m_timerReady.wait(lock, [this] {
                    return m_stopping.load() || !m_timerQueue.empty();
                });
                continue;
            }

            const auto deadline = m_timerQueue.top().m_wakeUpTime;
            if (deadline > std::chrono::steady_clock::now())
            {
                m_timerReady.wait_until(lock, deadline);
                continue;
            }

            auto event = m_timerQueue.top();
            m_timerQueue.pop();
            lock.unlock();
            event.m_dispatch(); // 위치/방향은 읽지 않고 기존 유저 FIFO에 작업만 추가한다.
            lock.lock();
        }

        m_timerQueue = {};
    }
    catch (const std::exception& _error)
    {
        m_workerFailed.store(true);
        LogPrinter::PrintMsg(std::string("Timer worker failed: ") + _error.what());
        wod::core::ProcessStopSignal::Request(GetCurrentProcessId());
    }
}

void CNetworkMgr::PrepareStop()
{
    {
        std::lock_guard lock(m_timerMutex);
        m_stopping.store(true);
    }

    m_timerReady.notify_all();
}

bool CNetworkMgr::RegisterEvent(TIMER_EVENT _event)
{
    {
        std::lock_guard lock(m_timerMutex);
        if (m_stopping.load())
            return false;

        m_timerQueue.push(std::move(_event));
    }

    m_timerReady.notify_one();

    return true;
}

void CNetworkMgr::DataBaseFunc()
{
    m_dataBaseThread->ThreadFunction();
}

const HANDLE& CNetworkMgr::GetHandle() const
{
    return m_handle->GetHandle();
}

void CNetworkMgr::ServerConnect(int _id, int _bytes, OverlapEx* _over_ex)
{
    m_gameServer->SetRemainData(0);
    m_gameServer->SetSocket(m_handle->GetClientSocket());

    NetworkRuntime::Get().Attach(m_handle->GetClientSocket(), static_cast<int>(m_gameServer->GetSocket()));
    m_gameServer->Recv();
    m_handle->GetOverEx().ResetOver();
    m_handle->GetOverEx().SetOP(OP_TYPE::OP_ACCEPT);
    SocketResource socketResource;
    Resource::m_socketpool.try_pop(socketResource);
    m_handle->Accept(socketResource.m_socket);
}

void CNetworkMgr::Accept(int _id, int _bytes, OverlapEx* _over_ex)
{

    const auto socket = m_handle->GetClientSocket();
    const int index = static_cast<int>(socket);
    SOCKADDR_IN address{};
    int length = sizeof(address);
    getpeername(socket, reinterpret_cast<SOCKADDR*>(&address), &length);
    char text[INET_ADDRSTRLEN]{};
    inet_ntop(AF_INET, &address.sin_addr, text, INET_ADDRSTRLEN);
    const bool admitted = m_clientNum.fetch_add(1) < MAX_CLIENT;
    GLobbyDispatcher->PostGlobal([index, ip = std::string(text), admitted] {
        if (admitted)
        {
            CUserMgr::GetInstance()->InitializeClient(index, ip);
        }
        else
        {
            auto* client = CUserMgr::GetInstance()->GetClient(index);
            client->Initialize(index);
            client->Disconnect();
        }
    });
    m_handle->GetOverEx().ResetOver();

    SocketResource socketResource;
    if (Resource::m_socketpool.try_pop(socketResource))
    {
        m_handle->Accept(socketResource.m_socket);
    }
    else
    {
        SOCKET s = NetworkRuntime::Get().CreateSocket();
        NetworkRuntime::Get().Attach(s, static_cast<int>(s));
        CUserMgr::GetInstance()->MakeClientObject(static_cast<int>(s));
        m_handle->Accept(s);
    }
}

void CNetworkMgr::Recv(int _id, int _bytes, OverlapEx* _overEx)
{

    if (_id != m_gameServer->GetSocket())
    {
        auto* client = CUserMgr::GetInstance()->GetClient(_id);
        const auto session = client->GetPacketSender()->GetSession();
        session->WithGeneration(session->Generation(), [&] {
            client->Receive(_bytes, _overEx);
        });
        return;
    }
    std::vector<wod::core::FrameDecoder::Frame> frames;
    if (!m_gameServer->Decode(_bytes, *_overEx, frames))
    {
        LogPrinter::PrintMsg("Invalid game frame");
        NetworkRuntime::Get().Close(m_gameServer->GetSocket());
        return;
    }
    for (auto& frame : frames)
    {
        if (!wod::protocol::Validate(frame, wod::protocol::Endpoint::GameToLobby))
        {
            LogPrinter::PrintMsg("Invalid game packet");
            NetworkRuntime::Get().Close(m_gameServer->GetSocket());
            return;
        }
        GLobbyDispatcher->PostGlobal([this, frame = std::move(frame)]() mutable {
            PacketExec(reinterpret_cast<BASE_PACKET*>(frame.data()));
        });
    }
    m_gameServer->Recv();
}

void CNetworkMgr::Send(int _id, int _bytes, OverlapEx* _overEx)
{
    Resource::m_overExPool.push(_overEx);
}

void CNetworkMgr::Disconnect(int _id, int _bytes, OverlapEx* _overEx)
{
    CClient* client = CUserMgr::GetInstance()->GetClient(_id);
    client->QueueDisconnect();

    _overEx->Reset();
    Resource::m_overExPool.push(_overEx);

    --m_clientNum;
}

void CNetworkMgr::PacketExec(BASE_PACKET* _packet)
{
    //Game Server Packet Execution
    switch (_packet->type)
    {
    case GL_TRANSACTIONS: {
        GL_TRANSACTIONS_PACKET* p = reinterpret_cast<GL_TRANSACTIONS_PACKET*>(_packet);
#ifdef WITH_DATABASE
        m_dataBaseThread->RegisterDBEvent(DB_EVENT(std::string(p->name), p->token, std::chrono::system_clock::now(), DB_EVENT_TYPE::EV_SAVE_TOKEN));
#endif

#ifdef WITH_DATABASE
        TransactionData transaction;
        memcpy_s(transaction.name, NAME_SIZE, p->name, NAME_SIZE);
        transaction.token = p->token;
        memcpy_s(transaction.time, NAME_SIZE, p->time, NAME_SIZE);

        m_p2pNetwork->InsertTransaction(transaction);
#endif
    }
    break;
    }
}
}
