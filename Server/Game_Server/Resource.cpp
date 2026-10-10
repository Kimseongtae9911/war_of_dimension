#include "pch.h"
#include "Resource.h"
#include "TCPSocket.h"

namespace wod_server {
	CSessionPool Resource::m_sessionPool;
	wod::core::ObjectPool<OverlapEx> Resource::m_overExPool;

	OverlapEx* Resource::GetOverObjectFromPool()
	{
		return m_overExPool.Acquire();
	}

	void CSessionPool::push(const std::shared_ptr<Session>& _session)
	{
		_session->m_entryTime = TimeUtil::CurTime();
		m_sessionPool.push(_session);
	}

	bool CSessionPool::try_pop(std::shared_ptr<Session>& _session)
	{
		return m_sessionPool.try_pop(_session);
	}

}