#pragma once

namespace wod_server {
#define PACKET_SENDER(id) CObjectMgr::GetInstance()->GetClient(id)->GetPacketSender()

using TimePoint = std::chrono::system_clock::time_point;
}