#include "pch.h"
#include "CUserMgr.h"
#include "CMatchMgr.h"
#include "CNetworkMgr.h"

namespace wod_server
{
std::unique_ptr<CUserMgr> CUserMgr::m_instance;

bool CUserMgr::Initialize()
{
    return true;
}

bool CUserMgr::Release()
{
    for (auto& entry : m_clients)
        delete entry.second;

    m_clients.clear();

    return true;
}

void CUserMgr::InitializeClient(int _index, const std::string& _ip)
{
    auto* client = GetClient(_index);
    client->Initialize(_index);
    network::GetInstance()->GetP2PNetwork()->RegisterPeer(_index, _ip);
    if (!RegisterClientToChannel(client))
    {
        client->Disconnect();
        return;
    }

    client->GetPacketSender()->GetSession()->Recv();
    client->GetPacketSender()->SendRTTPacket();
}

void CUserMgr::DisconnectClient(int _key)
{
    GetClient(_key)->Disconnect();
}

bool CUserMgr::ClaimAccount(const std::string& _name, CClient* _client)
{
    if (!m_accountOwners.Claim(_name, _client))
        return false;

    _client->SetAccountName(_name);

    return true;
}

void CUserMgr::ReleaseAccount(CClient* _client)
{
    m_accountOwners.Release(_client->GetAccountName(), _client);
    _client->SetAccountName({});
}

void CUserMgr::PublishBalance(const std::string& _name, const PlayerInfo& _balance)
{
    auto* owner = static_cast<CClient*>(m_accountOwners.Find(_name));
    if (owner && !owner->IsDisconnected() && owner->ApplyBalance(_balance))
    {
        const auto state = owner->GetState();
        if (state == CL_STATE::ST_LOBBY || state == CL_STATE::ST_MATCHING)
            owner->GetPacketSender()->SendTokenNumPacket(owner->GetPlayerInfo().m_tokenNum);
    }
}

void CUserMgr::RemoveFromViews(CClient* _client)
{
    const int id = _client->GetID();
    const int socket = _client->GetSocketID();
    for (int observerSocket : _client->GetViewList()->GetView())
    {
        auto* observer = GetClient(observerSocket);
        observer->GetViewList()->DeleteFromView(id, socket, observer->GetPacketSender());
    }

    _client->GetViewList()->ClearViewList();
}

void CUserMgr::ClientReset(int _index)
{
    auto* client = GetClient(_index);
    LogPrinter::PrintMsg(std::string(client->GetName()) + " Disconnect");
    ReleaseAccount(client);
    RemoveFromViews(client);
    CMatchMgr::GetInstance()->Cancel(_index);
    {
        std::lock_guard lock(m_channelLock);
        const int channel = client->GetChannel();
        if (channel >= 0 && channel < CHANNEL_NUM && client->GetID() >= 0)
            m_channelClient[channel][client->GetID() % LOBBY_CHANNEL_CAPACITY] = nullptr;
    }

    if (!client->Reset())
        throw std::runtime_error("Client reset failed");
}

bool CUserMgr::RegisterClientToChannel(CClient* _client)
{
    int channel = -1;
    {
        std::lock_guard lock(m_channelLock);
        if (_client->GetChannel() >= 0)
            return true;

        for (int i = 0; i < CHANNEL_NUM && channel < 0; ++i)
        {
            for (int j = 0; j < LOBBY_CHANNEL_CAPACITY; ++j)
            {
                if (m_channelClient[i][j])
                    continue;

                _client->SetChannel(i);
                _client->SetID(i * LOBBY_CHANNEL_CAPACITY + j);
                m_channelClient[i][j] = _client;
                channel = i;
                break;
            }
        }
    }

    if (channel < 0)
        return false;

    _client->GetPacketSender()->SendChangeChannelPacket(false, channel);

    return true;
}

bool CUserMgr::ChangeChannel(int _curChannel, int _changeChannel, CClient* _client)
{
    if (_curChannel < 0 || _curChannel >= CHANNEL_NUM || _changeChannel < 0 || _changeChannel >= CHANNEL_NUM || _client->GetState() == CL_STATE::ST_MATCHING)
        return false;

    if (_curChannel == _changeChannel)
        return true;

    int slot = -1;
    {
        std::lock_guard lock(m_channelLock);
        for (int i = 0; i < LOBBY_CHANNEL_CAPACITY; ++i)
        {
            if (!m_channelClient[_changeChannel][i])
            {
                slot = i;
                break;
            }
        }
    }

    if (slot < 0)
        return false;

    // 호출자의 전역 배타 구간에서 완료한다. 기존 ID가 유효할 때 관찰자에게 알린다.
    RemoveFromViews(_client);
    {
        std::lock_guard lock(m_channelLock);
        m_channelClient[_curChannel][_client->GetID() % LOBBY_CHANNEL_CAPACITY] = nullptr;
        _client->SetID(_changeChannel * LOBBY_CHANNEL_CAPACITY + slot);
        _client->SetChannel(_changeChannel);
        m_channelClient[_changeChannel][slot] = _client;
    }

    return true;
}
}
