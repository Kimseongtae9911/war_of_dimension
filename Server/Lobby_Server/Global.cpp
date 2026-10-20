#include "pch.h"

namespace wod_server
{
namespace
{
LobbyDispatcher dispatcher(CHANNEL_NUM);
}

LobbyDispatcher* GLobbyDispatcher = &dispatcher;
}
