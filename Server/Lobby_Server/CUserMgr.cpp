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

	void CUserMgr::InitializeClient(int index)
	{
		m_clients[index]->Initialize(index);

		RegisterClientToChannel(m_clients[index]);
		m_clients[index]->GetPacketSender()->GetSession()->Recv();
		m_clients[index]->GetPacketSender()->SendRTTPacket();
	}

	void CUserMgr::DisconnectClient(int key)
	{
        m_clients.at(key)->Disconnect();
	}

	void CUserMgr::ClientReset(int index)
	{
		LogPrinter::PrintMsg(std::string(m_clients[index]->GetName()) + " Disconnect");
		m_channelClient[m_clients[index]->GetChannel()][m_clients[index]->GetID() % LOBBY_MAX_CLIENT] = nullptr;

		if (!m_clients[index]->Reset()) {
			LogPrinter::PrintMsg("Client Reset Fail");
		}
	}

	void CUserMgr::RegisterClientToChannel(CClient* client)
	{
		for (int i = 0; i < CHANNEL_NUM; ++i) {
			for (int j = 0; j < LOBBY_MAX_CLIENT; ++j) {
				m_channelLock.lock();
				if (m_channelClient[i][j] == nullptr) {
					client->SetChannel(i);
					client->SetID(i * LOBBY_MAX_CLIENT + j);
					m_channelClient[i][j] = client;
					m_channelLock.unlock();
					client->GetPacketSender()->SendChangeChannelPacket(false, i);
					return;
				}
				else {
					m_channelLock.unlock();
				}
			}
		}
	}

	bool CUserMgr::ChangeChannel(int curChannel, int changeChannel, CClient* client)
	{
		if (changeChannel >= CHANNEL_NUM)
			return false;

		for (int i = 0; i < LOBBY_MAX_CLIENT; ++i) {
			m_channelLock.lock();
			if (!m_channelClient[changeChannel][i]) {
				for (int id : client->GetViewList()->GetView()) {
					CUserMgr::GetInstance()->GetClient(id)->GetViewList()->DeleteFromView(client->GetID(), static_cast<int>(client->GetPacketSender()->GetSession()->GetSocket()), client->GetPacketSender());
				}
				client->GetViewList()->ClearViewList();

				m_channelClient[curChannel][client->GetID() % LOBBY_MAX_CLIENT] = nullptr;
				int prevID = client->GetID();
				client->SetID(changeChannel * LOBBY_MAX_CLIENT + i);
				client->SetChannel(changeChannel);
				m_channelClient[changeChannel][i] = client;
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