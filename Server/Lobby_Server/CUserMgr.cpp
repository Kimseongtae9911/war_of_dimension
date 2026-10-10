#include "pch.h"
#include "CUserMgr.h"
#include "LogUtil.h"
#include "SocketUtil.h"
#include "Resource.h"

namespace wod_server {
	std::unique_ptr<CUserMgr> CUserMgr::m_instance;

	bool CUserMgr::Initialize()
	{
		return true;
	}

	bool CUserMgr::Release()
	{
		try {
			for (auto& iter : m_clients) {
				delete iter.second;
			}

            m_clients.clear();
			return true;
		}
		catch (std::exception ex) {
			LogPrinter::PrintMsg("Err(UserMgr Release): " + std::string(ex.what()));
			return false;
		}
	}

	void CUserMgr::InitializeClient(int _index)
	{
		m_clients[_index]->Initialize(_index);

		RegisterClientToChannel(m_clients[_index]);
		m_clients[_index]->GetPacketSender()->GetSession()->Recv();
		m_clients[_index]->GetPacketSender()->SendRTTPacket();
	}

	void CUserMgr::DisconnectClient(int _key)
	{
        m_clients.at(_key)->Disconnect();
	}

	void CUserMgr::ClientReset(int _index)
	{
		LogPrinter::PrintMsg(std::string(m_clients[_index]->GetName()) + " Disconnect");
		m_channelClient[m_clients[_index]->GetChannel()][m_clients[_index]->GetID() % LOBBY_MAX_CLIENT] = nullptr;

		if (!m_clients[_index]->Reset()) {
			LogPrinter::PrintMsg("Client Reset Fail");
		}
	}

	void CUserMgr::RegisterClientToChannel(CClient* _client)
	{
		for (int i = 0; i < CHANNEL_NUM; ++i) {
			for (int j = 0; j < LOBBY_MAX_CLIENT; ++j) {
				m_channelLock.lock();
				if (m_channelClient[i][j] == nullptr) {
					_client->SetChannel(i);
					_client->SetID(i * LOBBY_MAX_CLIENT + j);
					m_channelClient[i][j] = _client;
					m_channelLock.unlock();
					_client->GetPacketSender()->SendChangeChannelPacket(false, i);
					return;
				}
				else {
					m_channelLock.unlock();
				}
			}
		}
	}

	bool CUserMgr::ChangeChannel(int _curChannel, int _changeChannel, CClient* _client)
	{
		if (_changeChannel >= CHANNEL_NUM)
			return false;

		for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
			m_channelLock.lock();
			if (!m_channelClient[_changeChannel][i]) {
				for (int id : _client->GetViewList()->GetView()) {
					CUserMgr::GetInstance()->GetClient(id)->GetViewList()->DeleteFromView(_client->GetID(), static_cast<int>(_client->GetPacketSender()->GetSession()->GetSocket()), _client->GetPacketSender());
				}
				_client->GetViewList()->ClearViewList();

				m_channelClient[_curChannel][_client->GetID() % LOBBY_MAX_CLIENT] = nullptr;
				int prevID = _client->GetID();
				_client->SetID(_changeChannel * LOBBY_MAX_CLIENT + i);
				_client->SetChannel(_changeChannel);
				m_channelClient[_changeChannel][i] = _client;
				m_channelLock.unlock();

				return true;
			}
			else {
				m_channelLock.unlock();
			}
		}

		return false;
	}
}