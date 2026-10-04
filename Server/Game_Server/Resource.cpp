#include "pch.h"
#include "Resource.h"
#include "TCPSocket.h"

namespace wod_server {
	CSessionPool Resource::sessionPool;
	concurrency::concurrent_priority_queue<OverlapEx*> Resource::overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		OverlapEx* overEx;
		if (!overExPool.try_pop(overEx)) {
			overEx = new OverlapEx;
		}
		return overEx;
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