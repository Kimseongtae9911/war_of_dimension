#pragma once
#include <unordered_set>
#include "CPacketSender.h"
#include "CTransform.h"
#include "CPhysic.h"
#include "CViewList.h"
#include <ServerCore/SessionHandler.h>
#include <ServerCore/Concurrency.h>

namespace wod_server
{
class CDataBase;

struct PlayerInfo
{
    ModelCustomize m_model;
    int m_tokenNum = 0;

    void Reset()
    {
        m_tokenNum = 0;
        memset(&m_model, 0, sizeof(m_model));
    }
};

class CClient : public std::enable_shared_from_this<CClient>, public wod::core::SessionHandler<Session>
{
  public:
    static constexpr auto m_movementInterval = std::chrono::milliseconds(15);
    CClient();
    ~CClient();

    void Initialize(const SOCKET& _socket);
    void QueueDisconnect();
    void RunDatabase(std::function<std::function<void(CClient*)>(CDataBase&)> _query);

    bool IsDisconnected() const
    {
        return m_disconnected.load();
    }

    void ClearWorkQueue()
    {
        m_workQueue.Clear();
    }

    void Move(bool _broadcast = true);
    void ApplyMovementInput(char _direction);
    bool Reset();

  public:
    CPacketSender* GetPacketSender() const
    {
        return m_packetSender.get();
    }

    CL_STATE GetState() const
    {
        return m_state;
    }

    const PlayerInfo& GetPlayerInfo() const
    {
        return m_playerInfo;
    }

    const ModelCustomize& GetModelCustomize() const
    {
        return m_playerInfo.m_model;
    }

    int GetSocketID() const
    {
        return m_socketID;
    }

    const std::string& GetIP() const
    {
        return m_ipAddress;
    }

    bool IsFullNode()
    {
        return m_isFullNode;
    }

    int GetID() const
    {
        return m_id;
    }

    const char* GetName() const
    {
        return m_name;
    }

    int GetChannel() const
    {
        return m_channel;
    }

    CTransform* GetTransform()
    {
        return m_transform;
    }

    CPhysic* GetPhysics()
    {
        return m_physics;
    }

    CViewList* GetViewList()
    {
        return m_viewList;
    }

    void SetState(CL_STATE _st)
    {
        m_state = _st;
    }

    void SetUpdateTime()
    {
        m_updateTime = std::chrono::steady_clock::now();
    }

    void SetPlayerInfo(const PlayerInfo& _info)
    {
        m_playerInfo = _info;
    }

    void SetTokenNum(int _num)
    {
        m_playerInfo.m_tokenNum = _num;
    }

    void SetModelCustomize(const ModelCustomize& _model)
    {
        m_playerInfo.m_model = _model;
    }

    void SetIP(const std::string& _ip)
    {
        m_ipAddress = _ip;
    }

    void SetFullNode(bool _fullNode)
    {
        m_isFullNode = _fullNode;
    }

    void SetChannel(int _channel)
    {
        m_channel = _channel;
        m_workQueue.SetChannel(_channel);
    }

    void SetSection(int _x, int _z)
    {
        m_sectionX = _x;
        m_sectionZ = _z;
    }

    void SetID(const int _id)
    {
        m_id = _id;
    }

    void SetName(const char* _name)
    {
        strncpy_s(m_name, NAME_SIZE, _name, _TRUNCATE);
    }

    void ProcessUpdate(bool _isDummy = false);

    std::shared_mutex m_stateLock;

  private:
    SessionRef GetTransportSession() const override;
    bool ValidateFrame(std::span<const char> _frame) const override;
    bool DispatchFrame(Frame _frame, const SessionRef& _session, uint64_t _generation) override;
    void CleanupDisconnected();
    void PublishMovement();
    void ScheduleMovementUpdate();
    void EnqueueMovementUpdate(const SessionRef& _session, uint64_t _generation, std::chrono::steady_clock::time_point _deadline);
    void ExecuteScheduledMovementUpdate(const SessionRef& _session, uint64_t _generation, std::chrono::steady_clock::time_point _dispatched);
    void RunMovementUpdate();

    std::unique_ptr<CPacketSender> m_packetSender;

    CL_STATE m_state = CL_STATE::ST_FREE;

    std::chrono::steady_clock::time_point m_updateTime = {};
    // 소유 채널 또는 전역 배타 작업에서만 변경한다. 타이머는 작업 큐에 전달한다.
    LobbyDispatcher::SessionWorkQueue m_workQueue;
    std::atomic_bool m_disconnected = true;
    std::atomic_bool m_cleanupScheduled = false;
    bool m_updateScheduled = false;
    bool m_movementDirty = false;
    bool m_isDummy = false;

    int m_socketID = -1;
    PlayerInfo m_playerInfo;

    int m_id = -1;
    char m_name[NAME_SIZE] = {};

    int m_channel = -1;
    int m_sectionX = -1;
    int m_sectionZ = -1;

    CTransform* m_transform;
    CPhysic* m_physics;
    CViewList* m_viewList;

    //P2P
    std::string m_ipAddress;
    bool m_isFullNode = false;
};
}
