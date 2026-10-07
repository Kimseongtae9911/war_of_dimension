#include "pch.h"
#include "Resource.h"

namespace wod_server {
	CSocketPool Resource::socketpool;
	wod::core::ObjectPool<OverlapEx> Resource::overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		return overExPool.Acquire();
	}
}