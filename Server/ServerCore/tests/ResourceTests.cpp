#include <ServerCore/Resource.h>
#include <array>

using namespace wod::core;

namespace
{
struct ContextA : IoContext
{
};

struct ContextB : IoContext
{
};

struct Session
{
    std::chrono::system_clock::time_point m_entryTime{};
};

using GameResources = SessionResources<ContextA, Session>;
using LobbyResources = SocketResources<ContextB>;

void Check(bool _condition, const char *_message, int& _checks)
{
    ++_checks;
    if (!_condition)
        throw std::runtime_error(_message);
}
}

int RunResourceTests()
{
    int checks = 0;
    auto *gameContext = GameResources::GetOverObjectFromPool();
    Check(GameResources::m_overExPool.Leased() == 1 && IoResources<ContextA>::m_overExPool.Leased() == 1, "resource configuration shares context pool", checks);
    Check(LobbyResources::m_overExPool.Leased() == 0, "different context resource pools isolated", checks);
    bool refused = false;
    try
    {
        GameResources::m_overExPool.Clear();
    }
    catch (const std::logic_error&)
    {
        refused = true;
    }
    Check(refused, "resource pool refuses clearing leased context", checks);
    IoResources<ContextA>::m_overExPool.push(gameContext);
    auto *reused = GameResources::GetOverObjectFromPool();
    Check(reused == gameContext && GameResources::m_overExPool.Size() == 1, "resource facade reuses returned context", checks);
    GameResources::m_overExPool.push(reused);
    GameResources::m_overExPool.Clear();

    auto session = std::make_shared<Session>();
    const auto before = std::chrono::system_clock::now();
    GameResources::m_acceptSessionPool.push(session);
    Check(session->m_entryTime >= before, "session pool records entry time", checks);
    std::shared_ptr<Session> result;
    Check(!GameResources::m_sessionPool.try_pop(result), "accept and reuse session pools isolated", checks);
    Check(GameResources::m_acceptSessionPool.try_pop(result) && result == session, "session pool retains shared owner identity", checks);
    GameResources::m_sessionPool.push(result);
    result.reset();
    Check(GameResources::m_sessionPool.try_pop(result) && result == session && !GameResources::m_sessionPool.try_pop(result), "session reusable exactly once", checks);

    SocketResource older(SOCKET{10}), newer(SOCKET{20});
    older.m_entryTime = {};
    newer.m_entryTime = older.m_entryTime + std::chrono::seconds(1);
    concurrency::concurrent_priority_queue<SocketResource> ordered;
    ordered.push(newer);
    ordered.push(older);
    SocketResource socket;
    Check(ordered.try_pop(socket) && socket.m_socket == older.m_socket, "socket comparison preserves oldest entry first", checks);
    Check(ordered.try_pop(socket) && socket.m_socket == newer.m_socket, "socket comparison returns remaining entry", checks);
    LobbyResources::m_socketpool.push(SOCKET{30});
    Check(LobbyResources::m_socketpool.try_pop(socket) && socket.m_socket == SOCKET{30} && !LobbyResources::m_socketpool.try_pop(socket), "socket configuration preserves handle once", checks);
    Check(GameResources::m_overExPool.Leased() == 0 && LobbyResources::m_overExPool.Leased() == 0, "resource tests return all contexts", checks);

    return checks;
}
