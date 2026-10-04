#include "pch.h"
#include "Zone.h"

namespace wod_server {
	void Zone::Initialize()
	{
	}

	void Zone::Release()
	{
	}

	ZONE_STATE Zone::Update()
	{
		if (m_clients.empty())
			return ZONE_STATE::READY; // 클라이언트가 없으면 업데이트 필요 없음

		m_jobQueue.ProcessJob(); // 작업 큐 처리

		for(auto client : m_clients)
			client->ProcessUpdate(false); // 클라이언트 업데이트 처리


		return ZONE_STATE::UPDATE; // 클라이언트가 있으면 업데이트 필요
	}
}