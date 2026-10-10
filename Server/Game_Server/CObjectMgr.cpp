#include "pch.h"
#include "CObjectMgr.h"
#include "TCPSocket.h"

namespace wod_server {
	std::unique_ptr<CObjectMgr> CObjectMgr::m_instance;

	bool CObjectMgr::Initialize()
	{
		try {
			for (int i = 0; i < MAX_CLIENT; ++i) {
				m_clientIDQueue.push(i);
			}

			for (int i = 0; i < m_npcs.size(); ++i) {
				for (int j = 0; j < MAX_MINION; ++j) {
					m_npcs[i][j] = std::make_shared<CMinion>();
					m_npcs[i][j]->SetMatchNum(i);
					m_npcs[i][j]->SetID(NPC_ID + j);
				}
			}

			for (int i = 0; i < m_npcs.size(); ++i) {
				//Unique
				m_npcs[i][MAX_MINION] = std::make_shared<CUniqueRed>();
				m_npcs[i][MAX_MINION]->SetMatchNum(i);
				m_npcs[i][MAX_MINION]->SetID(NPC_ID + MAX_MINION);

				//Rare
				m_npcs[i][MAX_MINION + 1] = std::make_shared<CRareGreen>();
				m_npcs[i][MAX_MINION + 1]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 1]->SetID(NPC_ID + MAX_MINION + 1);

				m_npcs[i][MAX_MINION + 2] = std::make_shared<CRareGolem>();
				m_npcs[i][MAX_MINION + 2]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 2]->SetID(NPC_ID + MAX_MINION + 2);

				//Normal
				m_npcs[i][MAX_MINION + 3] = std::make_shared<CNormalBear>();
				m_npcs[i][MAX_MINION + 3]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 3]->SetID(NPC_ID + MAX_MINION + 3);

				m_npcs[i][MAX_MINION + 4] = std::make_shared<CNormalMinotaur>();
				m_npcs[i][MAX_MINION + 4]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 4]->SetID(NPC_ID + MAX_MINION + 4);

				m_npcs[i][MAX_MINION + 5] = std::make_shared<CNormalChest>();
				m_npcs[i][MAX_MINION + 5]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 5]->SetID(NPC_ID + MAX_MINION + 5);

				m_npcs[i][MAX_MINION + 6] = std::make_shared<CNormalBeholder>();
				m_npcs[i][MAX_MINION + 6]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 6]->SetID(NPC_ID + MAX_MINION + 6);

				m_npcs[i][MAX_MINION + 7] = std::make_shared<CNormalChest>();
				m_npcs[i][MAX_MINION + 7]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 7]->SetID(NPC_ID + MAX_MINION + 7);

				m_npcs[i][MAX_MINION + 8] = std::make_shared<CNormalBeholder>();
				m_npcs[i][MAX_MINION + 8]->SetMatchNum(i);
				m_npcs[i][MAX_MINION + 8]->SetID(NPC_ID + MAX_MINION + 8);
			}
			return true;
		}
		catch (const std::exception& ex) {
			LogPrinter::PrintMsg("Err(CObjectMgr Initialize), " + std::string(ex.what()));
			return false;
		}
	}

	bool CObjectMgr::Release()
	{
        m_clients.clear();
        for(auto& match : m_npcs) for(auto& npc : match) npc.reset();
        m_clientnames.clear(); return true;
	}

	void CObjectMgr::DisconnectClient(int _key)
	{
		m_infolock[_key].lock_shared();
		m_clients[m_idInfo[_key]]->Disconnect();
		m_infolock[_key].unlock_shared();
	}

	bool CObjectMgr::InitializeClient(const SOCKET& _socket, int _socketID)
	{
		int clientID = -1;
		if (!m_clientIDQueue.try_pop(clientID)) {
			return false;
		}
		m_clients[clientID]->GetPacketSender()->GetSession()->SetSocket(_socket);
		m_clients[clientID]->Initialize();
		m_clients[clientID]->SetID(clientID);
		m_clients[clientID]->GetPacketSender()->GetSession()->Recv();

		m_infolock[_socketID].lock();
		m_idInfo[_socketID] = clientID;
		m_infolock[_socketID].unlock();
		m_clients[clientID]->GetPacketSender()->SendRTTPacket();

		return true;
	}

	void CObjectMgr::RegisterClientToServer(char _name[NAME_SIZE], int _id, int _matchNum, const ModelCustomize& _model)
	{
		m_namelock.lock();
		m_clientnames.insert({ _name, UserDataFromLobby{ _id, _matchNum, _model } });
		m_namelock.unlock();
	}

	void CObjectMgr::RemoveClientFromServer(int _socketID)
	{
		int clid = GetUserIDFromSocket(_socketID);

		Resource::m_sessionPool.push(m_clients[clid]->GetPacketSender()->GetSession());

		m_namelock.lock();
		if (m_clientnames.contains(m_clients[clid]->GetName()))
			m_clientnames.erase(m_clients[clid]->GetName());
		m_namelock.unlock();
	}

	void CObjectMgr::RemoveClientFromServerByID(int _id)
	{
		Resource::m_sessionPool.push(m_clients[_id]->GetPacketSender()->GetSession());

		m_namelock.lock();
		if (m_clientnames.contains(m_clients[_id]->GetName()))
			m_clientnames.erase(m_clients[_id]->GetName());
		m_namelock.unlock();
	}

	const UserDataFromLobby CObjectMgr::GetUserData(const std::string& _name)
	{
		UserDataFromLobby data;
		m_namelock.lock_shared();
		if (m_clientnames.contains(_name)) {
			data = m_clientnames.find(_name)->second;
		}
		m_namelock.unlock_shared();
		return data;
	}

}