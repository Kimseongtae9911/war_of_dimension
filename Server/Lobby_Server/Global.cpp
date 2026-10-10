#include "pch.h"


namespace wod_server {
PacketJobQueue* GPacketJobQueue = nullptr;

class Global
{
public:
	Global()
	{
		GPacketJobQueue = new PacketJobQueue();
	}

	~Global()
	{
		delete GPacketJobQueue;
	}
} Global;
}