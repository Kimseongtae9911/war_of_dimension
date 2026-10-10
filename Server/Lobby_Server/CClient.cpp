#include "pch.h"
#include <Protocol/Validation.h>
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


		m_playerInfo.m_model = { 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
		1 , 0, 1 , 3 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 };
	}

	CClient::~CClient()
	{
		delete m_transform;
		delete m_physics;
		delete m_viewList;
		delete m_jobQueue;
	}

	void CClient::Initialize(const SOCKET& _socket)
	{
		m_packetSender->Initailize(_socket);
        m_isDisconnected.store(false);
		m_socketID = static_cast<int>(_socket);

		m_stateLock.lock();
		m_state = CL_STATE::ST_ALLOC;
		m_stateLock.unlock();

		m_sectionX = static_cast<int>((m_transform->GetPos().m_x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
		m_sectionZ = static_cast<int>((m_transform->GetPos().m_z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
		GameUtil::RegisterClientToSection(m_sectionX, m_sectionZ, static_cast<int>(_socket));
	}

	void CClient::RecvPacket(int _recvBytes, OverlapEx* _overEx)
	{

        auto session = m_packetSender->GetSession();
        const auto generation = session->Generation();
        std::vector<wod::core::FrameDecoder::Frame> frames;
        if (!session->Decode(_recvBytes, *_overEx, frames)) { Disconnect(); return; }
        for (auto& frame : frames) {
            if (!wod::protocol::Validate(frame, wod::protocol::Endpoint::LobbyClient)) { Disconnect(); return; }
            m_jobQueue->PushJob([this, session, generation, frame = std::move(frame)]() mutable {
                session->WithGeneration(generation, [&] { CPacketMgr::GetInstance()->Packet_Exec(reinterpret_cast<BASE_PACKET*>(frame.data()), this); });
            });
        }
        session->Recv();
        if (!frames.empty()) GPacketJobQueue->AddSessionQueue(this);

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
		int sectionX = static_cast<int>((newPos.m_x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
		int sectionZ = static_cast<int>((newPos.m_z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
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
			m_stateLock.lock();
			m_state = CL_STATE::ST_FREE;
			m_stateLock.unlock();

			m_viewList->ClearViewList();

			m_socketID = -1;
            m_packetSender->GetSession()->Invalidate();
            m_jobQueue->Clear();
			m_packetSender->Reset();
			m_transform->Reset();
			m_id = -1;
			m_channel = -1;
			m_playerInfo.Reset();
			m_updateTime = std::chrono::system_clock::now();
			memset(m_name, 0, sizeof(m_name));

			Resource::m_socketpool.push(m_packetSender->GetSession()->GetSocket());
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg(ex.what());
			return false;
		}
		return true;
	}

	void CClient::ProcessUpdate(bool _isDummy)
	{
		Move();

		if (_isDummy)
			m_packetSender->SendDummyMovePacket(m_id, m_transform->GetPos(), m_transform->GetDir());

		if (m_transform->GetDir() != 0)
		{
			m_jobQueue->PushJob([this, _isDummy]() {	ProcessUpdate(_isDummy); });
			GPacketJobQueue->AddSessionQueue(this);
		}
	}

	void CClient::Disconnect()
	{
        auto session = m_packetSender->GetSession();
        session->Invalidate();
        auto* over = Resource::GetOverObjectFromPool();
        over->SetOP(OP_TYPE::OP_DISCONNECT);
        if (!SocketUtil::Runtime().Disconnect(session->GetSocket(), *over)) Resource::m_overExPool.push(over);

	}
}