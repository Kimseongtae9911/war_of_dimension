#include "pch.h"
#include "Resource.h"
#include "TCPSocket.h"

namespace wod_server {
	CSessionPool Resource::sessionPool;
	wod::core::ObjectPool<OverlapEx> Resource::overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		return overExPool.Acquire();
	}

	void CSessionPool::push(const std::shared_ptr<Session>& session)
	{
		session->entryTime = TimeUtil::CurTime();
		sessionPool.push(session);
	}

	bool CSessionPool::try_pop(std::shared_ptr<Session>& session)
	{
		return sessionPool.try_pop(session);
	}

}