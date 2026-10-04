#include "pch.h"
#include "Resource.h"

namespace wod_server {
	CSocketPool Resource::socketpool;
	concurrency::concurrent_priority_queue<OverlapEx*> Resource::overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		OverlapEx* overEx;
		if (!overExPool.try_pop(overEx)) {
			overEx = new OverlapEx;
		}
		return overEx;
	}
}