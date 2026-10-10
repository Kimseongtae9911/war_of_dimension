#include "pch.h"
#include "Resource.h"

namespace wod_server {
	CSocketPool Resource::m_socketpool;
	wod::core::ObjectPool<OverlapEx> Resource::m_overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		return m_overExPool.Acquire();
	}
}