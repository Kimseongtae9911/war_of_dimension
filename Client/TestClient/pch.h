#pragma once
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <stdio.h>

#include "../../Server/Game_Server/protocol.h"

#pragma comment(lib, "WS2_32.lib")
#pragma comment(lib, "MSWSock.lib")

enum class OP_TYPE { OP_ACCEPT, OP_RECV, OP_SEND, OP_DISCONNECT, OP_CONNECT};