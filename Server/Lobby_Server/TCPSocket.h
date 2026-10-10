#pragma once
#include <ServerCore/Session.h>

namespace wod_server
{
class Session : public wod::core::PooledSession<OverlapEx, Resource, LOBBY_PORT>
{
  public:
    using PooledSession::PooledSession;
};

class TCPSocket : public wod::core::PooledListener<OverlapEx, Resource>
{
};
}
