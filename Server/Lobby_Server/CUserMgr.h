#pragma once

namespace wod_server {

	class CUserMgr : public TSingleton<CUserMgr>
	{
	public:
		bool Initialize() override;
		bool Release() override;

		void MakeClientObject(int _key) { m_clients.insert({ _key, new CClient }); }
		void InitializeClient(int _index);
		void DisconnectClient(int _key);
		void ClientReset(int _index);

		void RegisterClientToChannel(CClient* _client);
		bool ChangeChannel(int _curChannel, int _changeChannel, CClient* _client);

		CClient* GetClient(int _id) { return m_clients.at(_id); }
		const concurrency::concurrent_unordered_map<int, CClient*>& GetAllClient() const { return m_clients; }
        std::vector<CClient*> GetChannelClients(int _channel)
        {
            std::lock_guard lock(m_channelLock);

            return m_channelClient.at(_channel);
        }

	private:
		concurrency::concurrent_unordered_map<int, CClient*> m_clients;

		std::mutex m_channelLock;
        std::vector<std::vector<CClient*>> m_channelClient = std::vector<std::vector<CClient*>>(CHANNEL_NUM, std::vector<CClient*>(LOBBY_CHANNEL_CAPACITY, nullptr));
	};

}
