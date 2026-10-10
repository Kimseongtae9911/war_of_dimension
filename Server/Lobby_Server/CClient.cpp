#include "pch.h"
#include <Protocol/Validation.h>
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "CUserMgr.h"

namespace wod_server {

	CClient::CClient() : JobTarget(wod::core::JobBudget::Five)
	{
		m_packetSender = std::make_unique<CPacketSender>();
		m_transform = new CTransform();
		m_physics = new CPhysic();
		m_viewList = new CViewList(this);

		m_playerInfo.m_model = { 0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1,
		1 , 0, 1 , 3 , 2 , 2 , 2 , 2 , 1 , 1 , 1 , 1 , 1 };
	}

	CClient::~CClient()
	{
		delete m_transform;
		delete m_physics;
		delete m_viewList;
	}

	void CClient::Initialize(const SOCKET& _socket)
	{
		m_packetSender->Initailize(_socket);
        ResetDisconnected();
		m_socketID = static_cast<int>(_socket);

		m_stateLock.lock();
		m_state = CL_STATE::ST_ALLOC;
		m_stateLock.unlock();

		m_sectionX = static_cast<int>((m_transform->GetPos().m_x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
		m_sectionZ = static_cast<int>((m_transform->GetPos().m_z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
		GameUtil::RegisterClientToSection(m_sectionX, m_sectionZ, static_cast<int>(_socket));
	}

    CClient::SessionRef CClient::GetTransportSession() const
    {
        return m_packetSender->GetSession();
    }

    bool CClient::ValidateFrame(std::span<const char> _frame) const
    {
        return wod::protocol::Validate(_frame, wod::protocol::Endpoint::LobbyClient);
    }

    bool CClient::DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation)
    {
        GetJobQueue()->PushJob([this, session = _session, generation = _generation, frame = std::move(_frame)]() mutable {
            session->WithGeneration(generation, [&] {
                CPacketMgr::GetInstance()->Packet_Exec(reinterpret_cast<BASE_PACKET *>(frame.data()), this);
            });
        });

        return true;
    }

    void CClient::OnReceiveComplete(size_t _frames)
    {
        if (_frames)
            GPacketJobQueue->AddSessionQueue(this);
    }

    void CClient::OnJobQueueDisconnected()
    {
        CUserMgr::GetInstance()->ClientReset(GetSocketID());
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
            GetJobQueue()->Clear();
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
			GetJobQueue()->PushJob([this, _isDummy]() {	ProcessUpdate(_isDummy); });
			GPacketJobQueue->AddSessionQueue(this);
		}
	}

}
