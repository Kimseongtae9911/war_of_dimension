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
		std::array<CClient*, LOBBY_MAX_CLIENT> GetChannelClients(int _channel) { m_channelLock.lock(); std::array<CClient*, LOBBY_MAX_CLIENT> temp = m_channelClient[_channel]; m_channelLock.unlock(); return temp; }

	private:
		concurrency::concurrent_unordered_map<int, CClient*> m_clients;

		std::mutex m_channelLock;
		std::array<std::array<CClient*, LOBBY_MAX_CLIENT>, CHANNEL_NUM> m_channelClient;
	};

}