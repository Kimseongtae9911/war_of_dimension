#pragma once

namespace wod_server {

	class CUserMgr : public TSingleton<CUserMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void MakeClientObject(int key) { m_clients.insert({ key, new CClient }); }
		void InitializeClient(int index);
		void DisconnectClient(int key);
		void ClientReset(int index);

		void RegisterClientToChannel(CClient* client);
		bool ChangeChannel(int curChannel, int changeChannel, CClient* client);

		CClient* GetClient(int id) { return m_clients.at(id); }
		const concurrency::concurrent_unordered_map<int, CClient*>& GetAllClient() const { return m_clients; }
		std::array<CClient*, LOBBY_MAX_CLIENT> GetChannelClients(int channel) { m_channelLock.lock(); std::array<CClient*, LOBBY_MAX_CLIENT> temp = m_channelClient[channel]; m_channelLock.unlock(); return temp; }

	private:
		concurrency::concurrent_unordered_map<int, CClient*> m_clients;

		std::mutex m_channelLock;
		std::array<std::array<CClient*, LOBBY_MAX_CLIENT>, CHANNEL_NUM> m_channelClient;
	};

}