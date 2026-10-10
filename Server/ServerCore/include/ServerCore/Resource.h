#pragma once
#include <ServerCore/Concurrency.h>
#include <ServerCore/Net.h>

namespace wod::core
{
// context 타입별로 하나의 풀을 공유한다. native 소켓 소유권은 IocpService에 있다.
template <class Context> class IoResources
{
  public:
    static Context *GetOverObjectFromPool()
    {
        return m_overExPool.Acquire();
    }

    inline static ObjectPool<Context> m_overExPool;
};

template <class Session> class SessionPool
{
  public:
    void push(const std::shared_ptr<Session>& _session)
    {
        _session->m_entryTime = std::chrono::system_clock::now();
        m_sessions.push(_session);
    }

    bool try_pop(std::shared_ptr<Session>& _session)
    {
        return m_sessions.try_pop(_session);
    }

  private:
    // 기존 shared_ptr 비교를 유지한다. 시간순/FIFO 계약이 아니다.
    concurrency::concurrent_priority_queue<std::shared_ptr<Session>> m_sessions;
};

struct SocketResource
{
    SocketResource() = default;

    explicit SocketResource(const SOCKET& _socket) : m_socket(_socket), m_entryTime(std::chrono::system_clock::now())
    {
    }

    bool operator<(const SocketResource& _other) const
    {
        return m_entryTime > _other.m_entryTime;
    }

    SOCKET m_socket = INVALID_SOCKET;
    std::chrono::system_clock::time_point m_entryTime{};
};

class SocketPool
{
  public:
    void push(const SOCKET& _socket)
    {
        m_sockets.push(SocketResource(_socket));
    }

    bool try_pop(SocketResource& _socket)
    {
        return m_sockets.try_pop(_socket);
    }

  private:
    concurrency::concurrent_priority_queue<SocketResource> m_sockets;
};

template <class Context, class Session> class SessionResources : public IoResources<Context>
{
  public:
    inline static SessionPool<Session> m_sessionPool;
    inline static SessionPool<Session> m_acceptSessionPool;
};

template <class Context> class SocketResources : public IoResources<Context>
{
  public:
    inline static SocketPool m_socketpool;
};
}
