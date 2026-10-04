#include "pch.h"
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "CUserMgr.h"
#include "SocketUtil.h"
#include "Resource.h"

namespace wod_server {

	CClient::CClient()
	{
		m_packetSender = std::make_unique<CPacketSender>();
		m_transform = new CTransform();
		m_physics = new CPhysic();
		m_viewList = new CViewList(this);
		
		m_jobQueue = new JobQueue();


		m_playerInfo.model = { 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
		1 , 0, 1 , 3 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 };
	}

	CClient::~CClient()
	{
		delete m_transform;
		delete m_physics;
		delete m_viewList;
		delete m_jobQueue;
	}

	void CClient::Initialize(const SOCKET& socket)
	{
		m_packetSender->Initailize(socket);
		m_socketID = static_cast<int>(socket);

		stateLock.lock();
		m_state = CL_STATE::ST_ALLOC;
		stateLock.unlock();

		m_sectionX = static_cast<int>((m_transform->GetPos().x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
		m_sectionZ = static_cast<int>((m_transform->GetPos().z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
		GameUtil::RegisterClientToSection(m_sectionX, m_sectionZ, static_cast<int>(socket));
	}

	void CClient::RecvPacket(int recvBytes, OverlapEx* overEx)
	{
		int remaindata = recvBytes + m_packetSender->GetSession()->GetRemainData();
		char* packet = overEx->GetSendBuf();

		bool needProcess = false;
		while (remaindata > 0) {
			BASE_PACKET* basePacket = reinterpret_cast<BASE_PACKET*>(packet);

			if (basePacket->size <= remaindata) {
				// 개선 필요. 매번 동적할당 일어남
				std::unique_ptr<char[]> packetCopy = std::make_unique<char[]>(basePacket->size);
				memcpy(packetCopy.get(), basePacket, basePacket->size);

				m_jobQueue->PushJob([this, p = std::move(packetCopy)]() {
					CPacketMgr::GetInstance()->Packet_Exec(reinterpret_cast<BASE_PACKET*>(p.get()), this);
					});
				
				packet += basePacket->size;
				remaindata -= basePacket->size;
				needProcess = true;
			}
			else
				break;
		}
		m_packetSender->GetSession()->SetRemainData(remaindata);
		if (remaindata > 0)
			memmove(overEx->GetSendBuf(), packet, remaindata);
		m_packetSender->GetSession()->Recv();

		if(needProcess)
			GPacketJobQueue->AddSessionQueue(this);
	}

	void CClient::Move()
	{
		long long elpasedTimeMicro = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::system_clock::now() - m_updateTime).count();
		if (elpasedTimeMicro <= 1000)
			return;

		float elapsedTime = elpasedTimeMicro * 0.000001f;
		SetUpdateTime();

		vec3 moveShift = m_physics->CalculateMoveShift(m_transform->GetDir(), m_transform->GetLook(), m_transform->GetRight()) * elapsedTime;
		float height;
		if (true == GameUtil::MapCollision(m_transform->GetPos() + moveShift, height)) {
			m_transform->Move(moveShift, height);
		}

		vec3 newPos = m_transform->GetPos();

		if (m_transform->GetDir() != 0) {
			m_packetSender->SendMovePacket(m_id, newPos, m_transform->GetDir());
		}

		m_physics->Deceleration(elapsedTime);

		//Update Section
		int sectionX = static_cast<int>((newPos.x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
		int sectionZ = static_cast<int>((newPos.z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
		if (m_sectionX != sectionX || m_sectionZ != sectionZ) {
			int beforeX = m_sectionX;
			int beforeZ = m_sectionZ;
			m_sectionX = sectionX;
			m_sectionZ = sectionZ;
			GameUtil::UpdateSection(m_socketID, beforeX, beforeZ, m_sectionX, m_sectionZ);
		}

		m_viewList->CheckViewList(m_channel, m_socketID, m_id);
	}

	bool CClient::Reset()
	{
		try {
			stateLock.lock();
			m_state = CL_STATE::ST_FREE;
			stateLock.unlock();

			m_viewList->ClearViewList();

			m_socketID = -1;
			m_packetSender->Reset();
			m_transform->Reset();
			m_id = -1;
			m_channel = -1;
			m_playerInfo.Reset();
			m_updateTime = std::chrono::system_clock::now();
			memset(m_name, 0, sizeof(m_name));

			Resource::socketpool.push(m_packetSender->GetSession()->GetSocket());
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg(ex.what());
			return false;
		}
		return true;
	}

	void CClient::ProcessUpdate(bool isDummy)
	{
		Move();

		if (isDummy)
			m_packetSender->SendDummyMovePacket(m_id, m_transform->GetPos(), m_transform->GetDir());

		if (m_transform->GetDir() != 0)
		{
			m_jobQueue->PushJob([this, isDummy]() {	ProcessUpdate(isDummy); });
			GPacketJobQueue->AddSessionQueue(this);
		}
	}

	void CClient::Disconnect()
	{
		OverlapEx* over = Resource::GetOverObjectFromPool();
		over->SetOP(OP_TYPE::OP_DISCONNECT);
		if (!SocketUtil::DisconnectEx(m_packetSender->GetSession()->GetSocket(), &over->GetOver(), TF_REUSE_SOCKET, NULL) &&
			WSA_IO_PENDING != WSAGetLastError() && ERROR_IO_PENDING != WSAGetLastError()) {
			SocketUtil::PrintError("Disconnect");
		}
	}
}