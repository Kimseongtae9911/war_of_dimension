#pragma once

#include "GameObject.h"
#include "CClient.h"
#include "CNpc.h"
#include "CMonster.h"
#include "CMinion.h"

namespace wod_server {

	struct UserDataFromLobby
	{
		int id = -1;
		int matchNum = -1;
		ModelCustomize model;
	};

	class CObjectMgr : public TSingleton<CObjectMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void MakeClientObject(int key) { m_clients.insert({ key, std::make_shared<CClient>() }); }
		void DisconnectClient(int key);
		bool InitializeClient(const SOCKET& socket, int socketID);

		void RegisterClientToServer(char name[NAME_SIZE], int id, int matchNum, const ModelCustomize& model);
		void RemoveClientFromServer(int socketID);
		void RemoveClientFromServerByID(int id);

		int GetUserIDFromSocket(int socketID) { infolock[socketID].lock_shared(); int clid = m_idInfo[socketID]; infolock[socketID].unlock_shared(); return clid; }
		const UserDataFromLobby GetUserData(const std::string& name);

		const std::shared_ptr<CClient> GetClient(int index) const { return m_clients.at(index); }
		const std::shared_ptr<CNpc> GetNpc(int matchNum, int index) const { return m_npcs[matchNum][index]; }

	public:
		std::shared_mutex namelock;
		std::array<std::shared_mutex, MAX_SOCKET> infolock;
		
	private:
		concurrency::concurrent_unordered_map<int, std::shared_ptr<CClient>> m_clients;
		std::array<std::array<std::shared_ptr<CNpc>, MAX_MINION + MONSTER_NUM>, MAX_MATCH> m_npcs;

		concurrency::concurrent_priority_queue<int> m_clientIDQueue;
		std::array<int, MAX_SOCKET> m_idInfo;
		std::unordered_map<std::string, UserDataFromLobby> m_clientnames;
	};

}
