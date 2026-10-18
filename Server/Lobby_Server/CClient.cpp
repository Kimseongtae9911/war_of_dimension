#include "pch.h"
#include <Protocol/Validation.h>
#include "CNetworkMgr.h"
#include "CPacketMgr.h"
#include "CUserMgr.h"

namespace wod_server
{

CClient::CClient() : JobTarget(wod::core::JobBudget::Five)
{
    m_packetSender = std::make_unique<CPacketSender>();
    m_transform = new CTransform();
    m_physics = new CPhysic();
    m_viewList = new CViewList(this);

    m_playerInfo.m_model = {0, 1, 0, 0, 1, 1, 3, 1, 0, 1, 0, 0, 1, 1, 1, 1, 0, 1, 3, 2, 2, 2, 2, 1, 1, 1, 1, 1};
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
    SetUpdateTime();
    m_updateScheduled = false;
    m_movementDirty = false;
    m_isDummy = false;

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
    GetJobQueue()->PushJob([this, session = _session, generation = _generation, frame = std::move(_frame), pending = wod::core::JobMetrics::TrackPacket()]() mutable {
        if (pending)
            pending->Start();

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

void CClient::Move(bool _broadcast)
{
    const auto now = std::chrono::steady_clock::now();
    const auto elapsedTimeMicro = std::chrono::duration_cast<std::chrono::microseconds>(now - m_updateTime).count();
    float elapsedTime = elapsedTimeMicro * 0.000001f;
    wod::core::TickProfile::Frame profile;
    if (_broadcast && wod::core::JobMetrics::Enabled())
    {
        wod::core::JobMetrics::m_lobbyGap.Record(static_cast<uint64_t>(elapsedTimeMicro));
        const auto targetUs = std::chrono::duration_cast<std::chrono::microseconds>(m_movementInterval).count();
        wod::core::JobMetrics::m_tickDelay.Record(static_cast<uint64_t>((std::max)(int64_t{0}, elapsedTimeMicro - targetUs)));
    }

    m_updateTime = now;

    vec3 moveShift = m_physics->CalculateMoveShift(m_transform->GetDir(), m_transform->GetLook(), m_transform->GetRight()) * elapsedTime;
    float height;
    if (true == GameUtil::MapCollision(m_transform->GetPos() + moveShift, height))
    {
        m_transform->Move(moveShift, height);
    }

    vec3 newPos = m_transform->GetPos();

    //Update Section
    int sectionX = static_cast<int>((newPos.m_x + (WORLD_WIDTH / 2)) / (WORLD_WIDTH / SECTION_NUM));
    int sectionZ = static_cast<int>((newPos.m_z + (WORLD_HEIGHT / 2)) / (WORLD_HEIGHT / SECTION_NUM));
    if (m_sectionX != sectionX || m_sectionZ != sectionZ)
    {
        int beforeX = m_sectionX;
        int beforeZ = m_sectionZ;
        m_sectionX = sectionX;
        m_sectionZ = sectionZ;
        GameUtil::UpdateSection(m_socketID, beforeX, beforeZ, m_sectionX, m_sectionZ);
    }

    if (_broadcast)
    {
        PublishMovement();
    }
}

bool CClient::Reset()
{
    try
    {
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
        SetUpdateTime();
        m_updateScheduled = false;
        m_movementDirty = false;
        m_isDummy = false;
        m_physics->SetVelocity(vec3(0, 0, 0));
        memset(m_name, 0, sizeof(m_name));

        Resource::m_socketpool.push(m_packetSender->GetSession()->GetSocket());
    }
    catch (std::exception ex)
    {
        LogPrinter::PrintMsg(ex.what());
        return false;
    }

    return true;
}

void CClient::ProcessUpdate(bool _isDummy)
{
    m_isDummy = _isDummy;
    Move();
    ScheduleMovementUpdate();
}

void CClient::PublishMovement()
{
    m_packetSender->SendMovePacket(m_id, m_transform->GetPos(), m_transform->GetDir());
    if (m_isDummy)
        m_packetSender->SendDummyMovePacket(m_id, m_transform->GetPos(), m_transform->GetDir());

    wod::core::TickProfile::Scope view(wod::core::TickProfile::Stage::LobbyView);
    m_viewList->CheckViewList(m_channel, m_socketID, m_id);
}

void CClient::ApplyMovementInput(char _direction)
{
    // 이전 방향으로 지난 시간까지만 적분한 뒤 새 입력을 적용한다.
    if (m_transform->GetDir() != 0)
    {
        Move(false);
    }
    else
    {
        SetUpdateTime();
    }

    m_transform->SetDir(_direction);
    if (_direction == 0)
        m_physics->SetVelocity(vec3(0, 0, 0));

    // 본인에게는 즉시 응답하고, 주변 유저의 변경분은 예약된 갱신에서 합친다.
    m_packetSender->SendMovePacket(m_id, m_transform->GetPos(), _direction);
    m_movementDirty = true;
    ScheduleMovementUpdate();
}

void CClient::ScheduleMovementUpdate()
{
    if (m_updateScheduled || (m_transform->GetDir() == 0 && !m_movementDirty) || IsDisconnected())
        return;

    const auto session = GetTransportSession();
    const auto generation = session->Generation();
    const auto deadline = std::chrono::steady_clock::now() + m_movementInterval;
    auto onTimer = [this, session, generation, deadline] {
        EnqueueMovementUpdate(session, generation, deadline);
    };
    m_updateScheduled = network::GetInstance()->RegisterEvent({deadline, std::move(onTimer)});
}

void CClient::EnqueueMovementUpdate(const SessionRef& _session, uint64_t _generation, std::chrono::steady_clock::time_point _deadline)
{
    const auto dispatched = std::chrono::steady_clock::now();
    if (wod::core::JobMetrics::Enabled())
        wod::core::JobMetrics::m_tickTimerLateness.Record(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(dispatched - _deadline).count()));

    GetJobQueue()->PushJob([this, session = _session, generation = _generation, dispatched] {
        ExecuteScheduledMovementUpdate(session, generation, dispatched);
    });
    GPacketJobQueue->AddSessionQueue(this);
}

void CClient::ExecuteScheduledMovementUpdate(const SessionRef& _session, uint64_t _generation, std::chrono::steady_clock::time_point _dispatched)
{
    _session->WithGeneration(_generation, [this, _dispatched] {
        if (wod::core::JobMetrics::Enabled())
            wod::core::JobMetrics::m_tickDispatchWait.Record(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - _dispatched).count()));

        RunMovementUpdate();
    });
}

void CClient::RunMovementUpdate()
{
    m_updateScheduled = false;
    if (IsDisconnected() || (m_state != CL_STATE::ST_LOBBY && m_state != CL_STATE::ST_MATCHING) || (m_transform->GetDir() == 0 && !m_movementDirty))
        return;

    Move();
    m_movementDirty = false;
    // 밀린 주기 수만큼 재등록하지 않는다. 실행 완료 뒤 다음 한 번만 예약한다.
    ScheduleMovementUpdate();
}

}
