#pragma once

#include "GameObject.h"
#include "CClient.h"
#include "CNpc.h"
#include "CMonster.h"
#include "CMinion.h"

namespace wod_server {

	struct UserDataFromLobby
	{
		int m_id = -1;
		int m_matchNum = -1;
		ModelCustomize m_model;
	};

	class CObjectMgr : public TSingleton<CObjectMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void MakeClientObject(int _key) { m_clients.insert({ _key, std::make_shared<CClient>() }); }
		void DisconnectClient(int _key);
		bool InitializeClient(const SOCKET& _socket, int _socketID);

		void RegisterClientToServer(char _name[NAME_SIZE], int _id, int _matchNum, const ModelCustomize& _model);
		void RemoveClientFromServer(int _socketID);
		void RemoveClientFromServerByID(int _id);

		int GetUserIDFromSocket(int _socketID) { m_infolock[_socketID].lock_shared(); int clid = m_idInfo[_socketID]; m_infolock[_socketID].unlock_shared(); return clid; }
		const UserDataFromLobby GetUserData(const std::string& _name);

		const std::shared_ptr<CClient> GetClient(int _index) const { return m_clients.at(_index); }
		const std::shared_ptr<CNpc> GetNpc(int _matchNum, int _index) const { return m_npcs[_matchNum][_index]; }

	public:
		std::shared_mutex m_namelock;
		std::array<std::shared_mutex, MAX_SOCKET> m_infolock;

	private:
		concurrency::concurrent_unordered_map<int, std::shared_ptr<CClient>> m_clients;
		std::array<std::array<std::shared_ptr<CNpc>, MAX_MINION + MONSTER_NUM>, MAX_MATCH> m_npcs;

		concurrency::concurrent_priority_queue<int> m_clientIDQueue;
		std::array<int, MAX_SOCKET> m_idInfo;
		std::unordered_map<std::string, UserDataFromLobby> m_clientnames;
	};

}
